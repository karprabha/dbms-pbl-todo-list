#include <iostream>
#include <string>
#include "sqlite3.h"

static int callback(void *count, int argc, char **argv, char **azColName) {
    int *c = (int*)count;
    *c = atoi(argv[0]);
    return 0;
}

void print_help(){
    std::cout<<"Usage :-"<<std::endl;
    std::cout<<"$ ./task add 2 hello world    # Add a new item with priority 2 and text \"hello world\" to the list"<<std::endl;
    std::cout<<"$ ./task ls                   # Show incomplete priority list items sorted by priority in ascending order"<<std::endl;
    std::cout<<"$ ./task del INDEX            # Delete the incomplete item with the given index"<<std::endl;
    std::cout<<"$ ./task done INDEX           # Mark the incomplete item with the given index as complete"<<std::endl;
    std::cout<<"$ ./task help                 # Show usage"<<std::endl;
    std::cout<<"$ ./task report               # Statistics"<<std::endl;
}

void initializeTable(sqlite3** db,std::string& tableDescription){
	char* error = NULL;
	std::string query = "CREATE TABLE IF NOT EXISTS " + tableDescription;

	sqlite3_open("todoDB.db",&(*db));
	int rc = sqlite3_exec((*db),query.c_str(),NULL,NULL,&error);

	if(rc != SQLITE_OK)std::cout<<"error : "<<error<<std::endl;
}

void view_pendingTasks(sqlite3** db,sqlite3_stmt** stmt){
	sqlite3_prepare_v2(*db,"SELECT priority, task_description FROM pending_tasks ORDER BY priority ASC",-1,&(*stmt),0);

	int idx = 1;
	int priority;
	const unsigned char* task_description;

	while(sqlite3_step(*stmt) != SQLITE_DONE){
		priority = sqlite3_column_int(*stmt,0);
		task_description = sqlite3_column_text(*stmt,1);
		std::cout<<idx++<<". "<<task_description<<" ["<<priority<<"]"<<std::endl;
	}
	sqlite3_reset(*stmt);
	sqlite3_finalize(*stmt);
	*stmt = NULL;
}

void view_completedTasks(sqlite3** db,sqlite3_stmt** stmt){
	sqlite3_prepare_v2(*db,"SELECT task_description FROM completed_tasks",-1,&(*stmt),0);

	int idx = 1;
	const unsigned char* task_description;

	while(sqlite3_step(*stmt) != SQLITE_DONE){
		task_description = sqlite3_column_text(*stmt,0);
		std::cout<<idx++<<". "<<task_description<<std::endl;
	}
	sqlite3_reset(*stmt);
	sqlite3_finalize(*stmt);
	*stmt = NULL;
}

void add_task(sqlite3** db,std::string& query){
	char* error = NULL;

	int rc = sqlite3_exec(*db,query.c_str(),NULL,NULL,&error);
	if(rc != SQLITE_OK)std::cout<<"error : "<<error<<std::endl;
}

void delete_task(sqlite3** db,int index){
	std::string querry = "DELETE FROM pending_tasks WHERE ROWID IN (SELECT ROWID FROM pending_tasks ORDER BY priority ASC LIMIT 1 OFFSET " + std::to_string(index-1) +")";
	char* error = NULL;

	int rc = sqlite3_exec(*db,querry.c_str(),NULL,NULL,&error);
	if(rc != SQLITE_OK)std::cout<<"error : "<<error<<std::endl;
}

void mark_task_done(sqlite3** db_pendingTask,sqlite3** db_completedTask,sqlite3_stmt** stmt,int index){
	//fetch task from pending task
	std::string query = "SELECT task_description FROM pending_tasks ORDER BY priority ASC LIMIT 1 OFFSET ?";
	std::string task_description;
	if(sqlite3_prepare_v2(*db_pendingTask,query.c_str(),-1,&(*stmt),0) == SQLITE_OK){
		sqlite3_bind_int(*stmt, 1, index-1);

		while(sqlite3_step(*stmt) != SQLITE_DONE)task_description = std::string(reinterpret_cast<char const*>(sqlite3_column_text(*stmt,0)));
	}
	sqlite3_reset(*stmt);
	sqlite3_finalize(*stmt);
	*stmt = NULL;
	
	//delete task in pending task
	delete_task(&(*db_pendingTask),index);

	//insert into completed task
	query = "INSERT INTO completed_tasks VALUES(\"" + task_description +"\")";
	add_task(&(*db_completedTask),query);
}

void show_statistics(sqlite3** db_pendingTask,sqlite3** db_completedTask,sqlite3_stmt** stmt){
	char* error = NULL;
	int rowCount = 0;

	std::string query = "SELECT COUNT(ROWID) from pending_tasks";
	int rc = sqlite3_exec(*db_pendingTask,query.c_str(),callback,&rowCount,&error);
	if(rc != SQLITE_OK)std::cout<<"error : "<<error<<std::endl;

    std::cout<<"Pending : "<<rowCount<<std::endl;
    view_pendingTasks(&(*db_pendingTask),&(*stmt));

	rowCount = 0;
	query = "SELECT COUNT(ROWID) from completed_tasks";
	rc = sqlite3_exec(*db_completedTask,query.c_str(),callback,&rowCount,&error);
	if(rc != SQLITE_OK)std::cout<<"error : "<<error<<std::endl;

    std::cout<<"\nCompleted : "<<rowCount<<std::endl;
    view_completedTasks(&(*db_completedTask),&(*stmt));
}


int main(int argc, char* argv[]){
	sqlite3* db_pendingTask = NULL;
	sqlite3* db_completedTask = NULL;
	sqlite3_stmt* stmt = NULL;
	std::string pendingTaskSchema = "pending_tasks(priority INT,task_description TEXT NOT NULL)";
	std::string completedTaskSchema = "completed_tasks(task_description TEXT NOT NULL)";

	initializeTable(&db_pendingTask,pendingTaskSchema);
	initializeTable(&db_completedTask,completedTaskSchema);


	if(argc == 1)
		print_help();
	else if(argc == 2){
		std::string command = argv[1];
		if(command=="help")
			print_help();
		else if(command == "ls")
			view_pendingTasks(&db_pendingTask,&stmt);
		else if(command == "report")
			show_statistics(&db_pendingTask,&db_completedTask,&stmt);
		else if(command == "done")
			std::cout<<"Error: Missing NUMBER for marking tasks as done."<<std::endl;
		else if(command == "del")
			std::cout<<"Error: Missing NUMBER for deleting tasks."<<std::endl;
		else if(command == "add")
			std::cout<<"Error: Missing tasks string. Nothing added!";
	}
	else if(argc == 3){
		std::string command[3];
		command[1] = argv[1];
		command[2] = argv[2];
		unsigned int NUMBER = std::stoi(command[2]);

		if(command[1]=="del"){
			char* error = NULL;
			int rowCount = 0;

			std::string query = "SELECT COUNT(ROWID) from pending_tasks";
			int rc = sqlite3_exec(db_pendingTask,query.c_str(),callback,&rowCount,&error);
			if(rc != SQLITE_OK)std::cout<<"error : "<<error<<std::endl;

			if(NUMBER<=rowCount&&NUMBER>0){
				delete_task(&db_pendingTask,NUMBER);
				std::cout<<"Deleted task #"<<NUMBER<<std::endl;
			}
			else
				std::cout<<"Error: task with index #"<<NUMBER<<" does not exist. Nothing deleted."<<std::endl;
		}
		else if(command[1]=="done"){
			char* error = NULL;
			int rowCount = 0;

			std::string query = "SELECT COUNT(ROWID) from pending_tasks";
			int rc = sqlite3_exec(db_pendingTask,query.c_str(),callback,&rowCount,&error);
			if(rc != SQLITE_OK)std::cout<<"error : "<<error<<std::endl;

			if(NUMBER<=rowCount&&NUMBER>0){
				mark_task_done(&db_pendingTask,&db_completedTask,&stmt,NUMBER);
				std::cout<<"Marked item as done."<<std::endl;
			}
			else
				std::cout<<"Error: no incomplete item with index #"<<NUMBER<<" exists."<<std::endl;
		}
		else if(command[1]=="add")
			std::cout<<"Error: Missing tasks string. Nothing added!";
	}
	else if(argc == 4){
		std::string command[4];
		command[1] = argv[1];
		command[2] = argv[2];
		command[3] = argv[3];
		int priority = std::stoi(command[2]);

		if(command[1]=="add"){
			std::string query = "INSERT INTO pending_tasks VALUES("+ std::to_string(priority) + ", \"" + command[3] +"\")";
			add_task(&db_pendingTask,query);
			std::cout<<" Added task: \""<<command[3]<<"\" with priority "<<priority<<std::endl;
		}
	}

	
    return 0;
}