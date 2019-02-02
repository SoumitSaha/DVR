#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>
#include <set>
#include <fstream>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#define INF 999999
#define UP -2
#define DOWN -3

using namespace std;
string this_router_ip;
int sockfd;
int no_of_bytes_received;
int clock_found;
set <string> routers_in_network;
bool updatedtable;

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
	int clk;
	int link_status;
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
				nbr.clk = 0;
				nbr.link_status = UP;
				neighbour_list.push_back(nbr);
			}
		}
		if(entry.r2.compare(this_router_ip) == 0){
			if(already_neighbour(entry.r1) == -1){
				struct neighbour nbr;
				nbr.ip = entry.r1;
				nbr.cost = entry.cost;
				nbr.clk = 0;
				nbr.link_status = UP;
				neighbour_list.push_back(nbr);
			}
		}
	}
	reader.close();
}


void make_initial_routing_table(string ip){
	struct table_entry entry;
	string temp_dest = "192.168.10.";
	string dest;
	for (int i = 0; i < routers_in_network.size(); ++i){
		char ch = i + 1 + '0';
		dest = temp_dest + ch;
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
	//cout << this_router_ip << endl;
	cout << "Destination  \tNext hop \tCost" << endl;
	cout << "-------------\t-------------\t-----" << endl;
	for (int i = 0; i < route_table.size(); ++i){
		if(!route_table[i].dest.compare(this_router_ip)) continue;
		cout << route_table[i].dest << "\t" << route_table[i].next_router << "\t" << route_table[i].cost << endl;
	}
}

void print_neighbours(){
	cout << this_router_ip << endl;
	cout << "Neighbours :" << endl;
	//cout << "-------------\t-------------\t-----" << endl;
	for (int i = 0; i < neighbour_list.size(); ++i){
		//if(!route_table[i].dest.compare(this_router_ip)) continue;
		cout << "ip : " << neighbour_list[i].ip << " Cost : " << neighbour_list[i].cost << endl;
	}
}

void print_routers(){
	set<string>::iterator it;
	cout << this_router_ip << "--->" << endl;
	for (it = routers_in_network.begin(); it != routers_in_network.end(); ++it){
		cout << *it << endl;
	}
}

void send_table_to_neighbours(string pkt){
	for (int i = 0; i < neighbour_list.size(); ++i){
		struct sockaddr_in neighbour_router;
		neighbour_router.sin_family = AF_INET;
		neighbour_router.sin_port = htons (4747);
		inet_pton(AF_INET, neighbour_list[i].ip.c_str(), &neighbour_router.sin_addr);
		sendto(sockfd, pkt.c_str(), 1024, 0, (struct sockaddr*) &neighbour_router, sizeof(sockaddr_in));
	}
}

string tbl_to_pkt(){
	string pkt = "tabl" + this_router_ip;
	for (int i = 0; i < route_table.size(); ++i){
		pkt += ":" + route_table[i].dest + "^" + route_table[i].next_router + "^" + to_string(route_table[i].cost);
	}
	return pkt;
}

void send_update_to_neighbour(){
	string pkt = tbl_to_pkt();
	send_table_to_neighbours(pkt);
}

void try_to_update_table(string ip, int i) {
	if(!ip.compare(route_table[i].next_router)){
		if(!ip.compare(route_table[i].dest) || (already_neighbour(route_table[i].dest) == false)){
			updatedtable = true;
			route_table[i].cost = INF;
			route_table[i].next_router = "\t-";
		}
		else if(already_neighbour(route_table[i].dest)){
			updatedtable = true;
			route_table[i].cost = neighbour_list[already_neighbour(route_table[i].dest)].cost;
			route_table[i].next_router = route_table[i].dest;
		}
	}
}

void update_table_link_fail(string ip){
	for (int i = 0; i < route_table.size(); ++i){
		try_to_update_table(ip, i);
	}
	if(updatedtable) print_route_table();
	updatedtable = false;
}

void link_down_detect_update_table(){
	for (int i = 0; i < neighbour_list.size(); ++i){
		if((clock_found - neighbour_list[i].clk) > 3 && neighbour_list[i].link_status == UP){
			cout << " Link down with : " << neighbour_list[i].ip << endl;
			neighbour_list[i].link_status = DOWN;
			update_table_link_fail(neighbour_list[i].ip);
		}
	}
}

vector<table_entry> pkt_to_tbl(string packet){
	vector<table_entry> table;
	char* str = new char[packet.length() + 1];
	strcpy(str, packet.c_str());
	char* token = strtok(str, ":");
	vector<string> entries;
	while(token != NULL){
		entries.push_back(token);
		token = strtok(NULL, ":");
	}
	struct table_entry single_entry;
	for (int i = 0; i < entries.size(); ++i){
		char* t = new char[entries[i].length() + 1];
		strcpy(t, entries[i].c_str());
		vector<string> myentry;
		char* tok = strtok(t, "^");
		while(tok != NULL){
			myentry.push_back(tok);
			tok = strtok(NULL, "^");
		}
		single_entry.dest = myentry[0];
		single_entry.next_router = myentry[1];
		single_entry.cost = atoi(myentry[2].c_str());
		myentry.clear();
		table.push_back(single_entry);
	}
	return table;
}

void try_to_update_on_clk(vector<table_entry> table, string ip){
	int temp;
	for (int i = 0; i < routers_in_network.size(); ++i){
		for (int j = 0; j < neighbour_list.size(); ++j){
			if(!ip.compare(neighbour_list[j].ip)){
				temp = neighbour_list[j].cost + table[i].cost;
				if (!ip.compare(route_table[i].next_router) || (temp < route_table[i].cost && this_router_ip.compare(table[i].next_router) != 0)){
					if (route_table[i].cost != temp){
						updatedtable = true;
						route_table[i].cost = temp;
						route_table[i].next_router = ip;
					}
					break;
				}
			}
		}
	}
	if(updatedtable) print_route_table();
	updatedtable = false;
}

string make_ip(unsigned char* ip){
	int ip_parts[4];
	for (int i = 0; i < 4; ++i){
		ip_parts[i] = ip[i];
	}
	string final_ip = to_string(ip_parts[0]) + "." + to_string(ip_parts[1]) + "." + to_string(ip_parts[2]) + "." + to_string(ip_parts[3]);
	return final_ip;
}

string cost_msg_neighbour(string sip1, string sip2, int changedCost){
	string nbr;
	for(int i = 0 ; i<neighbour_list.size(); i++){
        if(!sip1.compare(neighbour_list[i].ip)){
            //oldCost = neighbour_list[i].cost;
            neighbour_list[i].cost = changedCost;
            nbr = sip1;
        }
        else if(!sip2.compare(neighbour_list[i].ip)){
        	//oldCost = neighbour_list[i].cost;
            neighbour_list[i].cost = changedCost;
            nbr = sip2;
        }
    }
    return nbr;
}

int cost_msg_cost(string sip1, string sip2, int changedCost){
	int oldCost;
	for(int i = 0 ; i<neighbour_list.size(); i++){
        if(!sip1.compare(neighbour_list[i].ip)){
            oldCost = neighbour_list[i].cost;
            //neighbour_list[i].cost = changedCost;
            //nbr = sip1;
        }
        else if(!sip2.compare(neighbour_list[i].ip)){
        	oldCost = neighbour_list[i].cost;
            //neighbour_list[i].cost = changedCost;
            //nbr = sip2;
        }
    }
    return oldCost;
}

void try_to_update_on_cost(string nbr, int changedCost, int oldCost)
{
    for(int i = 0; i<routers_in_network.size(); i++){
        if(!nbr.compare(route_table[i].next_router)){
            if(!nbr.compare(route_table[i].dest)){
                route_table[i].cost = changedCost;
            }
            else{
                route_table[i].cost = route_table[i].cost - oldCost + changedCost;
            }
            updatedtable = true;
        }
		else if(!nbr.compare(route_table[i].dest) && route_table[i].cost>changedCost)
		{
			route_table[i].cost = changedCost;
			route_table[i].next_router = nbr;
			updatedtable = true;
		}
    }
    if(updatedtable == true) print_route_table();
	updatedtable = false;
}

void start_router(){
	struct sockaddr_in router;
	socklen_t address_length;
	clock_found = 0;
	updatedtable = false;
	while(true){
		char buffer[1024];
		no_of_bytes_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*) &router, &address_length);
		if(no_of_bytes_received != -1){
			string msg(buffer);
			//cout << msg << endl;
			string head = msg.substr(0,4);
			if(!head.compare("show")){
				print_route_table();
			}
			else if(!head.compare("cost")){
				unsigned char* ip1 = new unsigned char[5];
				unsigned char* ip2 = new unsigned char[5];
				string temp1 = msg.substr(4,4);
				string temp2 = msg.substr(8,4);
				for (int i = 0; i < 4; ++i){
					ip1[i] = temp1[i];
					ip2[i] = temp2[i];
				}
				string sip1 = make_ip(ip1);
				string sip2 = make_ip(ip2);
				unsigned char *c1 = new unsigned char[3];
				string tempCost = msg.substr(12,2);
				int changedCost = 0;
				c1[0] = tempCost[0];
				c1[1] = tempCost[1];
				int x0,x1;
				x0 = c1[0];
				x1 = c1[1]*256;
				changedCost = x1+x0;
				string nbr = cost_msg_neighbour(sip1, sip2, changedCost);
                int oldCost = cost_msg_cost(sip1, sip2, changedCost);
                try_to_update_on_cost(nbr, changedCost, oldCost);
			}
			else if(!head.compare("clk ")){
				clock_found++;
				send_update_to_neighbour();
				link_down_detect_update_table();
			}
			else if(!head.compare("tabl")){
				string got_from = msg.substr(4,12);
				//cout << "table from : " << got_from << endl;
				int neighbour_idx = already_neighbour(got_from);
				neighbour_list[neighbour_idx].link_status = UP;
				neighbour_list[neighbour_idx].clk = clock_found;
				int tbl_len = msg.length() - 15;
				char pkt[tbl_len];
				for (int i = 0; i < tbl_len; ++i){
					pkt[i] = buffer[16 + i];
				}
				string packet(pkt);
				vector<table_entry> neighbour_tbl;
				neighbour_tbl = pkt_to_tbl(packet);
				try_to_update_on_clk(neighbour_tbl, got_from);
			}

		}
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
	//print_routers();
	//print_neighbours();

	int bind_flag;
	struct sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_port = htons(4747);
	inet_pton(AF_INET, argv[1], &client.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bind_flag = bind(sockfd, (struct sockaddr*) &client, sizeof(sockaddr_in));
	if(!bind_flag) cout << "Connection Successful." << endl;
	else cout << "Connection failed." << endl;
	
	start_router();
	close(sockfd);
}