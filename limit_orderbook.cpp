#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <iomanip>
#include <stdlib.h>

class Order_Book {
private:
	struct order {
		double price;
		int num_shares;
		order(double p, int n) : price(p), num_shares(n) {}
	};

	struct compare_desc {
		bool operator() (const double& lhs, const double& rhs) const {
			return lhs > rhs;
		}
	};

	std::unordered_map<std::string, std::pair<order*, int>> mp_buy; //map from orderID to pointer to order object and number of shares in order
	std::unordered_map<std::string, std::pair<order*, int>> mp_sell;

	std::map<double, order*, compare_desc> s_buy; // map of orders sorted according to share price
	std::map<double, order*> s_sell;

	double expense = -1.0, income = -1.0;
	unsigned long target_size;
	bool is_buy_side;
	unsigned long total_buy_shares = 0, total_sell_shares = 0;
	//int i = 0;

	void add_order(std::vector<std::string>& message) {
		int new_shares = std::stoi(message[5]);
		double new_price = stod(message[4]);
		if (is_buy_side) {
			if (s_buy.count(new_price)) {
				mp_buy[message[2]] = std::make_pair(s_buy[new_price], new_shares);
				mp_buy[message[2]].first->num_shares += new_shares;
				total_buy_shares += new_shares;
				return;
			}
		}
		else {
			if (s_sell.count(new_price)) {
				mp_sell[message[2]] = std::make_pair(s_sell[new_price], new_shares);
				mp_sell[message[2]].first->num_shares += new_shares;
				total_sell_shares += new_shares;
				return;
			}
		}

		order* new_order = new order(new_price, new_shares); //price and number of shares
		is_buy_side ? s_buy[new_price] = new_order : s_sell[new_price] = new_order;
		is_buy_side ? mp_buy[message[2]] = std::make_pair(new_order, new_shares) : \
            mp_sell[message[2]] = std::make_pair(new_order, new_shares);
		is_buy_side ? total_buy_shares += new_shares : total_sell_shares += new_shares;
	}

	void reduce_order(std::vector<std::string>& message) {
		std::string order_id = message[2];
		int reduce_by = stoi(message[3]);
		if (is_buy_side) { // Buy side reduce
			total_buy_shares -= reduce_by;
			mp_buy[order_id].first->num_shares -= reduce_by;
			mp_buy[order_id].second -= reduce_by;

			if (mp_buy[order_id].first->num_shares == 0) {
				s_buy.erase(mp_buy[order_id].first->price);
				delete mp_buy[order_id].first;
				mp_buy.erase(order_id);
			}
			else if (mp_buy[order_id].second == 0)
				mp_buy.erase(order_id);
		}
		else { // Sell side reduce
			total_sell_shares -= reduce_by;
			mp_sell[order_id].first->num_shares -= reduce_by;
			if (mp_sell[order_id].first->num_shares == 0) {
				s_sell.erase(mp_sell[order_id].first->price);
				delete mp_sell[order_id].first;
				mp_sell.erase(order_id);
			}
			else if (mp_sell[order_id].second == 0)
				mp_sell.erase(order_id);
		}
	}

	std::vector<std::string> split(const std::string &text, char sep) {
		std::vector<std::string> tokens;
		std::size_t start = 0, end = 0;
		while ((end = text.find(sep, start)) != std::string::npos) {
			std::string temp = text.substr(start, end - start);
			if (temp != "") tokens.push_back(temp);
			start = end + 1;
		}
		std::string temp = text.substr(start);
		if (temp != "") tokens.push_back(temp);
		return tokens;
	}

	void display(const std::string& ts, const double& value) {
		char side = is_buy_side ? 'S' : 'B';
		if (value == -1.0)
			std::cout << ts << " " << side << " NA\n";
		else
			std::cout << ts << " " << side << " " << value << std::endl;
	}

	void calculate_and_display(const std::vector<std::string>& message) {
		bool is_lesser_than_target = is_buy_side ? total_buy_shares<target_size : total_sell_shares<target_size;
		if (is_lesser_than_target) {
			double* prev_inc_or_exp = is_buy_side ? &income : &expense;
			if (*prev_inc_or_exp != -1.0) {
				*prev_inc_or_exp = -1.0;
				display(message[0], *prev_inc_or_exp);
			}
		}
		else {
			unsigned long cur_size = 0;
			double cur_inc_or_exp = 0.0;
			double* prev_inc_or_exp = is_buy_side ? &income : &expense;

			auto it = is_buy_side ? s_buy.begin() : s_sell.begin();
			for (; cur_size<target_size; it++) {
				cur_inc_or_exp += it->second->price * \
                    std::min((unsigned long)it->second->num_shares, target_size - cur_size);
				cur_size += it->second->num_shares;
			}

			if (cur_inc_or_exp != *prev_inc_or_exp) {
				display(message[0], cur_inc_or_exp);
				*prev_inc_or_exp = cur_inc_or_exp;
			}
		}
	}
public:
	Order_Book(int target_size) {
		this->target_size = target_size;
	}
	void start() {
		std::fstream fs("pricer.in", std::fstream::in);
		std::string line;
		std::cout << std::fixed << std::setprecision(2);

		if (fs.is_open()) {
			while (1) {
				getline(fs, line);
                //std::getline( std::cin, line);
				is_buy_side = false;
				std::vector<std::string> message = split(line, ' ');
				if (line.length() < 10) {
					std::cout << std::endl;
					break;
				}
				if (line[9] == 'A') {
					// add order
					if (message[3] == "B") { // Buy side
						is_buy_side = true;
					}
					add_order(message);
				}
				else if(line[9] = 'R'){ // line[9] = 'R'
					// modify order
					if (mp_buy.count(message[2])) { // Buy side
						is_buy_side = true;
					}
					reduce_order(message);
				}

				// perform compute and display if income/expense change
				calculate_and_display(message);
			}
			fs.close();
		}
	}
	~Order_Book() {
		for (auto it = s_buy.begin(); it != s_buy.end(); it++) {
            delete it->second;
		}
		for (auto it = s_sell.begin(); it != s_sell.end(); it++) {
			delete it->second;
		}
	}
};

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cout << "Please input target size only" << std::endl;
		exit(1);
	}
	Order_Book* ob = new Order_Book(strtoul(argv[1], NULL, 10));
	ob->start();
	delete ob;
	return 0;
}
