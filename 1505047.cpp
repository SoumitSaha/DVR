#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>
#include <set>
#include <fstream>
#define INF -1

using namespace std;
string this_router_ip;

set <string> routers_in_network;

struct table_entry{
	string dest;
	string next_router;
	int cost;
};

struct file_entry{
	string r1;
	string r2;
	int cost;
};

struct neighbour{
	string ip;
	int cost;
};
vector <neighbour> neighbour_list;
vector <table_entry> route_table;
file_entry get_entry_from_file(ifstream file_reader){
	struct file_entry entry;
	file_reader >> entry.r1 >> entry.r2 >> entry.cost;
	return entry;
}

int already_neighbour(string router){
	for (int i = 0; i < neighbour_list.size(); ++i){
		if(!router.compare(neighbour_list[i].ip)){
			return i;
		}
	}
	return -1;
}

void list_neighbours(string ip, string topology_file){
	ifstream reader;
	reader.open(topology_file.c_str());
	while(!reader.eof()){
		struct file_entry entry;
		reader >> entry.r1 >> entry.r2 >> entry.cost;
		if(entry.r1.compare("")) {
			routers_in_network.insert(entry.r1);
		}
		if(entry.r2.compare("")) {
			routers_in_network.insert(entry.r2);
		}
		if(entry.r1.compare(this_router_ip) == 0){
			if(already_neighbour(entry.r2) == -1){
				struct neighbour nbr;
				nbr.ip = entry.r2;
				nbr.cost = entry.cost;
				neighbour_list.push_back(nbr);
			}
		}
		if(entry.r2.compare(this_router_ip) == 0){
			if(already_neighbour(entry.r1) == -1){
				struct neighbour nbr;
				nbr.ip = entry.r1;
				nbr.cost = entry.cost;
				neighbour_list.push_back(nbr);
			}
		}
	}
	reader.close();
}


void make_initial_routing_table(string ip){
	struct table_entry entry;
	string temp_dest = "192.168.10.";
	for (int i = 0; i < routers_in_network.size(); ++i){
		char ch = i + 1 + '0';
		string dest = temp_dest + ch;
		int is_neighbour = already_neighbour(dest);
		if(is_neighbour != -1){
			entry.dest = dest;
			entry.next_router = dest;
			entry.cost = neighbour_list[is_neighbour].cost;
		}
		else if(!this_router_ip.compare(dest)){
			entry.dest = dest;
			entry.next_router = dest;
			entry.cost = 0;
		}
		else{
			entry.dest = dest;
			entry.next_router = "\t-";
			entry.cost = INF;
		}
		route_table.push_back(entry);
	}
}

void print_route_table(){
	cout << this_router_ip << endl;
	cout << "Destination  \tNext hop \tCost" << endl;
	cout << "-------------\t-------------\t-----" << endl;
	for (int i = 0; i < route_table.size(); ++i){
		if(!route_table[i].dest.compare(this_router_ip)) continue;
		cout << route_table[i].dest << "\t" << route_table[i].next_router << "\t" << route_table[i].cost << endl;
	}
}

void print_routers(){
	set<string>::iterator it;
	cout << this_router_ip << "--->" << endl;
	for (it = routers_in_network.begin(); it != routers_in_network.end(); ++it){
		cout << *it << endl;
	}
}

int main(int argc, char* argv[]){
	
	if(argc != 3){
		cout << "Command Format: ./<cpp_file_name.cpp> <ip> <topology_file_name>" << endl;
		exit(1);
	}

	
	this_router_ip = argv[1];
	string topology_file;
	topology_file = argv[2];
	list_neighbours(this_router_ip, topology_file);
	make_initial_routing_table(this_router_ip);
	print_route_table();
}