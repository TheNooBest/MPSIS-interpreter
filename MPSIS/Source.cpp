#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <bitset>

using namespace std;

/* COMMAND STRUCT - total 25b

[0-2]
3b - jump type:
	000 - no jump
	001 - jne
	010 - jg
	011 - jl
	100 - je
	101 - jge
	110 - jle
	111 - jmp

[3-8]
SM settings [
	4b - S
	1b - M
	1b - P0
] 6b total

[9-16]
2b - ISR, ISL
1b - A
1b - wr
4b - v

[17-24]
4b - addr1
4b - addr2

COMMAND RULES:

* Symbol '!' mean constant, else mean addr
* Keyword 'in' mean DataIn
* Don't use addr 0
* Jump only on label

*/

class command {
public:
	string jmp = "000";
	string S = "0000";
	string M = "0";
	string P0 = "0";
	string in_shift = "00";
	string A = "0";
	string wr = "0";
	string v = "0000";
	string addr_rd = "0000";
	string addr_wr = "0000";

	command() {}
	command(string init) {
		jmp = init.substr(0, 3);
		S = init.substr(3, 4);
		M = init.substr(7, 1);
		P0 = init.substr(8, 1);
		in_shift = init.substr(9, 2);
		A = init.substr(11, 1);
		wr = init.substr(12, 1);
	}

	string result() { return jmp + S + P0 + in_shift + A + wr + v + addr_rd + addr_wr; }
};

map<string, vector<string>(*)(vector<string>&)> commands;
map<string, int> labels;
map<int, string> jmps;
int current_pos = 0;

vector<string> break_word(string& line) {
	vector<string> words;
	int space_pos = 0;
	int prev_pos = 0;
	while ((space_pos = (int)line.find(' ', prev_pos)) != string::npos) {
		words.push_back(line.substr(prev_pos, (uint64_t)space_pos - prev_pos));
		prev_pos = space_pos + 1;
	}
	string last = line.substr(prev_pos);
	words.push_back(last.substr(0, last.length() - 1));
	return words;
}

vector<string> jmp_body(string code, string& dest) {
	jmps[current_pos] = dest;
	current_pos++;

	command cmd;
	cmd.jmp = code;
	cmd.addr_rd = "";
	cmd.addr_wr = "";

	vector<string> result;
	result.push_back(cmd.result());
	return result;
}
vector<string> cmd_lda(string& addr) {
	current_pos++;

	command cmd;
	cmd.addr_rd = bitset<4>(stoi(addr)).to_string();
	cmd.v = "0001";

	vector<string> result;
	result.push_back(cmd.result());
	return result;
}
vector<string> cmd_ldb(string& addr) {
	current_pos++;

	command cmd;
	cmd.addr_rd = bitset<4>(stoi(addr)).to_string();
	cmd.v = "0110";

	vector<string> result;
	result.push_back(cmd.result());
	return result;
}
vector<command> cmd_const(string cnst) {
	current_pos += 3;

	vector<command> ret;
	bitset<4> bitnum = atoi(cnst.c_str() + 1);
	command cmd;
	cmd.v = "0010";

	for (int i = 3; i >= 0; i--) {
		cmd.in_shift[0] = bitnum[i] + '0';
		ret.push_back(cmd);
	}

	return ret;
}
vector<string> cmd_merge(vector<command>& cnst, command& cmd) {
	vector<string> result;
	result.push_back(cnst[0].result());

	cmd.in_shift = cnst[3].in_shift;
	cmd.v[1] = cnst[3].v[1];
	cmd.v[2] = cnst[3].v[2];
	cnst[3] = cmd;
	for (uint32_t i = 1; i < 4; i++) result.push_back(cnst[i].result());

	return result;
}

vector<string> cmd_nop(vector<string>& words) {
	current_pos++;
	command cmd;

	vector<string> result;
	result.push_back(cmd.result());
	return result;
}
vector<string> cmd_jne(vector<string>& words) {
	if (words.size() != 2) return vector<string>();
	return jmp_body("001", words[1]);
}
vector<string> cmd_jg(vector<string>& words) {
	if (words.size() != 2) return vector<string>();
	return jmp_body("010", words[1]);
}
vector<string> cmd_jl(vector<string>& words) {
	if (words.size() != 2) return vector<string>();
	return jmp_body("011", words[1]);
}
vector<string> cmd_je(vector<string>& words) {
	if (words.size() != 2) return vector<string>();
	return jmp_body("100", words[1]);
}
vector<string> cmd_jge(vector<string>& words) {
	if (words.size() != 2) return vector<string>();
	return jmp_body("101", words[1]);
}
vector<string> cmd_jle(vector<string>& words) {
	if (words.size() != 2) return vector<string>();
	return jmp_body("110", words[1]);
}
vector<string> cmd_jmp(vector<string>& words) {
	if (words.size() != 2) return vector<string>();
	return jmp_body("111", words[1]);
}
vector<string> cmd_mov(vector<string>& words) {
	current_pos++;

	if (words.size() != 3) return vector<string>();
	if (words[2][0] == '!' || words[2] == "in") return vector<string>();


	if (words[1][0] == '!') {
		bitset<4> to_wr = stoi(words[2]);
		if (to_wr == 0 && words[0] != "") return vector<string>();

		command cmd;
		cmd.S = "0101";
		cmd.wr = "1";
		cmd.addr_wr = to_wr.to_string();
		vector<command> cnst = cmd_const(words[1]);
		return cmd_merge(cnst, cmd);
	}
	else if (words[1] == "in") {
		bitset<4> to_wr = stoi(words[2]);
		if (to_wr == 0 && words[0] != "") return vector<string>();

		command cmd;
		cmd.A = "1";
		cmd.wr = "1";
		cmd.v = "0001";
		cmd.addr_wr = to_wr.to_string();

		vector<string> result;
		result.push_back(cmd.result());
		return result;
	}
	else {
		bitset<4> to_rd = stoi(words[1]);
		bitset<4> to_wr = stoi(words[2]);
		if (to_wr == 0 && words[0] != "") return vector<string>();

		command cmd;
		cmd.wr = "1";
		cmd.v = "0001";
		cmd.addr_wr = to_rd.to_string();
		cmd.addr_wr = to_wr.to_string();

		vector<string> result;
		result.push_back(cmd.result());
		return result;
	}
}
vector<string> cmd_add(vector<string>& words) {
	current_pos++;

	if (words.size() != 4) return vector<string>();
	if (words[3][0] == '!' || words[3] == "in") return vector<string>();
	
	if (words[1][0] == '!') {
		if (words[2][0] == '!') {
			// Const + Const -> too lazy :)
			return vector<string>();
		}
		else if (words[2] == "in") {
			// Const + DataIn
			bitset<4> to_wr = stoi(words[3]);
			if (to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "1001";
			cmd.M = "1";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_wr = to_wr.to_string();

			vector<command> cnst = cmd_const(words[1]);
			return cmd_merge(cnst, cmd);
		}
		else {
			// Const + Addr
			bitset<4> to_rd = stoi(words[2]);
			bitset<4> to_wr = stoi(words[3]);
			if (to_rd == 0 || to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "1001";
			cmd.M = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<command> cnst = cmd_const(words[1]);
			return cmd_merge(cnst, cmd);
		}
	}
	else if (words[1] == "in") {
		if (words[2][0] == '!') {
			// DataIn + Const
			bitset<4> to_wr = stoi(words[3]);
			if (to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "1001";
			cmd.M = "1";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0001";

			vector<command> cnst = cmd_const(words[2]);
			return cmd_merge(cnst, cmd);
		}
		else if (words[2] == "in") {
			// DataIn + DataIn
			bitset<4> to_wr = stoi(words[3]);
			if (to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "1001";
			cmd.M = "1";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_wr = to_wr.to_string();

			vector<string> _mov = { "", words[1], "0" };

			vector<string> result = cmd_mov(_mov);
			result.push_back(cmd.result());
			return result;
		}
		else {
			// DataIn + Addr
			bitset<4> to_rd = stoi(words[2]);
			bitset<4> to_wr = stoi(words[3]);
			if (to_rd == 0 || to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "1001";
			cmd.M = "1";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0111";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();


			vector<string> result;
			result.push_back(cmd.result());
			return result;
		}
	}
	else {
		if (words[2][0] == '!') {
			// Addr + Const
			bitset<4> to_rd = stoi(words[1]);
			bitset<4> to_wr = stoi(words[3]);
			if (to_rd == 0 || to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "1001";
			cmd.M = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<command> cnst = cmd_const(words[2]);
			return cmd_merge(cnst, cmd);
		}
		else if (words[2] == "in") {
			// Addr + DataIn
			bitset<4> to_rd = stoi(words[1]);
			bitset<4> to_wr = stoi(words[3]);
			if (to_rd == 0 || to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "1001";
			cmd.M = "1";
			cmd.wr = "1";
			cmd.v = "0111";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<string> result;
			result.push_back(cmd.result());
			return result;
		}
		else {
			// Addr + Addr
			bitset<4> to_rd = stoi(words[1]);
			bitset<4> to_wr = stoi(words[3]);
			if (to_rd == 0 || to_wr == 0 || stoi(words[2]) == 0) return vector<string>();

			command cmd;
			cmd.S = "1001";
			cmd.M = "1";
			cmd.wr = "1";
			cmd.v = "0110";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<string> result = cmd_lda(words[2]);
			result.push_back(cmd.result());
			return result;
		}
	}
}
vector<string> cmd_sub(vector<string>& words) {
	current_pos++;

	if (words.size() != 4) return vector<string>();
	if (words[3][0] == '!' || words[3] == "in") return vector<string>();

	// to lazy making Const - Smth :<
	if (words[1][0] == '!') {
		return vector<string>();
		if (words[2][0] == '!') {
			// Const - Const -> lazy again :P
		}
		else if (words[2] == "in") {
			// Const - DataIn
		}
		else {
			// Const - Addr
		}
	}
	else if (words[1] == "in") {
		if (words[2][0] == '!') {
			// DataIn - Const
			bitset<4> to_wr = stoi(words[3]);
			if (to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "0110";
			cmd.M = "1";
			cmd.P0 = "1";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_wr = to_wr.to_string();

			vector<command> _const = cmd_const(words[2]);
			return cmd_merge(_const, cmd);
		}
		else if (words[2] == "in") {
			// 4 steps :<
			return vector<string>();
			// DataIn - DataIn
			//bitset<4> to_wr = stoi(words[3]);
			//if (to_wr == 0) return vector<string>();

			//command cmd;
			//cmd.S = "0110";
			//cmd.M = "1";
			//cmd.P0 = "1";
			//cmd.A = "1";
			//cmd.wr = "1";
			//cmd.v = "0111";
			//cmd.addr_wr = to_wr.to_string();

			//vector<string> buff = { "", words[2], "0" };
			//return cmd_mov(buff) + "\n" + cmd.result();
		}
		else {
			// DataIn - Addr
			bitset<4> to_wr = stoi(words[3]);
			if (to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "0110";
			cmd.M = "1";
			cmd.P0 = "1";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_wr = to_wr.to_string();

			vector<command> _const = cmd_const(words[2]);
			return cmd_merge(_const, cmd);
		}
	}
	else {
		if (words[2][0] == '!') {
			// Addr - Const
			bitset<4> to_rd = stoi(words[1]);
			bitset<4> to_wr = stoi(words[3]);
			if (to_rd == 0 || to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "0110";
			cmd.M = "1";
			cmd.P0 = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_wr = to_wr.to_string();

			vector<command> _const = cmd_const(words[2]);
			return cmd_merge(_const, cmd);

		}
		else if (words[2] == "in") {
			// Addr - DataIn -> sub
			bitset<4> to_wr = stoi(words[3]);
			if (to_wr == 0) return vector<string>();

			command cmd;
			cmd.S = "0110";
			cmd.M = "1";
			cmd.P0 = "1";
			cmd.wr = "1";
			cmd.v = "0110";
			cmd.addr_wr = to_wr.to_string();

			vector<string> _mov = { "" , words[2], "0" };

			vector<string> result = cmd_mov(_mov);
			vector<string> _lda = cmd_lda(words[1]);
			result.insert(result.begin(), _lda.begin(), _lda.end());
			return result;
		}
		else {
			// Addr - Addr
			bitset<4> to_rd = stoi(words[2]);
			bitset<4> to_wr = stoi(words[3]);
			if (to_rd == 0 || to_wr == 0 || stoi(words[1]) == 0) return vector<string>();

			command cmd;
			cmd.S = "0110";
			cmd.M = "1";
			cmd.P0 = "1";
			cmd.wr = "1";
			cmd.v = "0110";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<string> result = cmd_lda(words[1]);
			result.push_back(cmd.result());
			return result;
		}
	}
}
vector<string> cmd_shr(vector<string>& words) {
	current_pos++;

	if (words.size() != 4) return vector<string>();
	if (words[1][0] == '!' || words[3][0] == '!' || words[1] == "in" || words[3] == "in") return vector<string>();

	bitset<4> to_wr = stoi(words[3]);
	if (to_wr == 0) return vector<string>();

	command cmd;
	cmd.S = "0101";
	cmd.wr = "1";
	cmd.v = "0100";
	cmd.addr_wr = to_wr.to_string();
	if (words[2] == "1") cmd.in_shift = "10";
	else if (words[2] != "0") return vector<string>();

	vector<string> result = cmd_ldb(words[1]);
	result.push_back(cmd.result());
	return result;
}
vector<string> cmd_shl(vector<string>& words) {
	current_pos++;

	if (words.size() != 4) return vector<string>();
	if (words[1][0] == '!' || words[3][0] == '!' || words[1] == "in" || words[3] == "in") return vector<string>();

	bitset<4> to_wr = stoi(words[3]);
	if (to_wr == 0) return vector<string>();

	command cmd;
	cmd.S = "0101";
	cmd.wr = "1";
	cmd.v = "0010";
	cmd.addr_wr = to_wr.to_string();
	if (words[2] == "1") cmd.in_shift = "01";
	else if (words[2] != "0") return vector<string>();

	vector<string> result = cmd_ldb(words[1]);
	result.push_back(cmd.result());
	return result;
}
//string cmd_inc(vector<string>& words) {
//	if (line[3] != ' ') return vector<string>();
//	string result = "000";
//	return result;
//}
//string cmd_dec(vector<string>& words) {
//	if (line[3] != ' ') return vector<string>();
//	string result = "000";
//	return result;
//}
vector<string> cmd_and(vector<string>& words) {
	current_pos++;

	if (words.size() != 4) return vector<string>();
	bitset<4> to_wr = stoi(words[3]);
	if (to_wr == 0) return vector<string>();
	command cmd;

	if (words[1][0] == '!') {
		if (words[1][0] == '!') {
			return vector<string>();

			// Const & Const
			//vector<string> _mov = { "", words[1], "0" };
		}
		else if (words[1] == "in") {
			// Const & DataIn
			cmd.S = "0100";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_wr = to_wr.to_string();

			vector<command> _const = cmd_const(words[1]);
			return cmd_merge(_const, cmd);
		}
		else {
			// Const & Addr
			bitset<4> to_rd = stoi(words[2]);
			if (to_rd == 0) return vector<string>();

			cmd.S = "0100";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<command> _const = cmd_const(words[1]);
			return cmd_merge(_const, cmd);
		}
	}
	else if (words[1] == "in") {
		if (words[1][0] == '!') {
			// DataIn & Const -> mov then and
			cmd.S = "0100";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_wr = to_wr.to_string();

			vector<command> _const = cmd_const(words[2]);
			return cmd_merge(_const, cmd);
		}
		else if (words[1] == "in") {
			// DataIn & DataIn
			cmd.S = "0100";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0111";
			cmd.addr_wr = to_wr.to_string();

			vector<string> _mov = { "", words[1], "0" };

			vector<string> result = cmd_mov(_mov);
			result.push_back(cmd.result());
			return result;
		}
		else {
			// DataIn & Addr -> and
			bitset<4> to_rd = stoi(words[2]);
			if (to_rd == 0) return vector<string>();

			cmd.S = "0100";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0111";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<string> _mov = { "", words[1], "0" };

			vector<string> result = cmd_mov(_mov);
			result.push_back(cmd.result());
			return result;
		}
	}
	else {
		if (words[1][0] == '!') {
			// Addr & Const
			bitset<4> to_rd = stoi(words[1]);
			if (to_rd == 0) return vector<string>();

			cmd.S = "0100";
			cmd.wr = "1";
			cmd.v = "0001";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<command> _const = cmd_const(words[2]);
			return cmd_merge(_const, cmd);
		}
		else if (words[1] == "in") {
			// Addr & DataIn
			bitset<4> to_rd = stoi(words[1]);
			if (to_rd == 0) return vector<string>();

			cmd.S = "0100";
			cmd.A = "1";
			cmd.wr = "1";
			cmd.v = "0111";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<string> _mov = { "", words[2], "0" };

			vector<string> result = cmd_mov(_mov);
			result.push_back(cmd.result());
			return result;
		}
		else {
			// Addr & Addr
			bitset<4> to_rd = stoi(words[1]);
			if (to_rd == 0 || stoi(words[2]) == 0) return vector<string>();

			cmd.S = "0100";
			cmd.wr = "1";
			cmd.v = "0110";
			cmd.addr_rd = to_rd.to_string();
			cmd.addr_wr = to_wr.to_string();

			vector<string> result = cmd_lda(words[2]);
			result.push_back(cmd.result());
			return result;
		}
	}
}
//string cmd_or(vector<string>& words) {
//	if (words.size() != 4) return vector<string>();
//	string result = "00000010000111110";
//	bitset<4> in, one, two;
//	if (words[1][0] == '!') {
//		if (words[1][0] == '!') {
//			// Const & Const -> mov then or
//			vector<string> buff = { "", words[1], "0" };
//			result = cmd_mov(buff) + "\n" + result;
//			in = atoi(words[2].c_str() + 1);
//			one = 0;
//		}
//		else if (words[1] == "in") {
//			// Const & DataIn -> mov then or
//			vector<string> buff = { "", words[2], "0" };
//			result = cmd_mov(buff) + "\n" + result;
//			in = atoi(words[1].c_str() + 1);
//			one = 0;
//		}
//		else {
//			// Const & Addr -> or
//			in = atoi(words[1].c_str() + 1);
//			one = stoi(words[2]);
//			if (one == 0) return vector<string>();
//		}
//	}
//	else if (words[1] == "in") {
//		if (words[1][0] == '!') {
//			// DataIn & Const -> mov then or
//			result[16] = '1';
//			vector<string> buff = { "", words[2], "0" };
//			result = cmd_mov(buff) + "\n" + result;
//			in = 0;
//			one = 0;
//		}
//		else if (words[1] == "in") {
//			// DataIn & DataIn -> mov then or
//			result[16] = '1';
//			vector<string> buff = { "", words[2], "0" };
//			result = cmd_mov(buff) + "\n" + result;
//			in = 0;
//			one = 0;
//		}
//		else {
//			// DataIn & Addr -> or
//			result[16] = '1';
//			in = 0;
//			one = stoi(words[2]);
//		}
//	}
//	else {
//		if (words[1][0] == '!') {
//			// Addr & Const -> or
//			in = atoi(words[2].c_str() + 1);
//			one = stoi(words[2]);
//			if (one == 0) return vector<string>();
//		}
//		else if (words[1] == "in") {
//			// Addr & DataIn -> or
//			result[16] = '1';
//			one = stoi(words[2]);
//			if (one == 0) return vector<string>();
//		}
//		else {
//			// Addr & Addr -> ld then or
//			result[11] = '0';
//			result[15] = '0';
//			if (stoi(words[2]) == 0) return vector<string>();
//			result = cmd_lda(words[2]) + "\n" + result;
//			in = 0;
//			one = stoi(words[1]);
//			if (one == 0) return vector<string>();
//		}
//	}
//	two = stoi(words[3]);
//	if (two == 0) return vector<string>();
//	return result + in.to_string() + one.to_string() + two.to_string();
//}
//string cmd_xor(vector<string>& words) {
//	if (words.size() != 4) return vector<string>();
//	string result = "00010010000111110";
//	bitset<4> in, one, two;
//	if (words[1][0] == '!') {
//		if (words[1][0] == '!') {
//			// Const & Const -> mov then xor
//			vector<string> buff = { "", words[1], "0" };
//			result = cmd_mov(buff) + "\n" + result;
//			in = atoi(words[2].c_str() + 1);
//			one = 0;
//		}
//		else if (words[1] == "in") {
//			// Const & DataIn -> mov then xor
//			vector<string> buff = { "", words[2], "0" };
//			result = cmd_mov(buff) + "\n" + result;
//			in = atoi(words[1].c_str() + 1);
//			one = 0;
//		}
//		else {
//			// Const & Addr -> xor
//			in = atoi(words[1].c_str() + 1);
//			one = stoi(words[2]);
//			if (one == 0) return vector<string>();
//		}
//	}
//	else if (words[1] == "in") {
//		if (words[1][0] == '!') {
//			// DataIn & Const -> mov then xor
//			result[16] = '1';
//			vector<string> buff = { "", words[2], "0" };
//			result = cmd_mov(buff) + "\n" + result;
//			in = 0;
//			one = 0;
//		}
//		else if (words[1] == "in") {
//			// DataIn & DataIn -> mov then xor
//			result[16] = '1';
//			vector<string> buff = { "", words[2], "0" };
//			result = cmd_mov(buff) + "\n" + result;
//			in = 0;
//			one = 0;
//		}
//		else {
//			// DataIn & Addr -> xor
//			result[16] = '1';
//			in = 0;
//			one = stoi(words[2]);
//		}
//	}
//	else {
//		if (words[1][0] == '!') {
//			// Addr & Const -> xor
//			in = atoi(words[2].c_str() + 1);
//			one = stoi(words[2]);
//			if (one == 0) return vector<string>();
//		}
//		else if (words[1] == "in") {
//			// Addr & DataIn -> xor
//			result[16] = '1';
//			one = stoi(words[2]);
//			if (one == 0) return vector<string>();
//		}
//		else {
//			// Addr & Addr -> ld then xor
//			result[11] = '0';
//			result[15] = '0';
//			if (stoi(words[2]) == 0) return vector<string>();
//			result = cmd_lda(words[2]) + "\n" + result;
//			in = 0;
//			one = stoi(words[1]);
//			if (one == 0) return vector<string>();
//		}
//	}
//	two = stoi(words[3]);
//	if (two == 0) return vector<string>();
//	return result + in.to_string() + one.to_string() + two.to_string();
//}
vector<string> cmd_not(vector<string>& words) {
	current_pos++;

	if (words.size() != 3) return vector<string>();
	if (words[2][0] == '!' || words[2] == "in") return vector<string>();

	bitset<4> to_wr = stoi(words[2]);
	if (to_wr == 0) return vector<string>();

	if (words[1][0] == '!') {

		command cmd;
		cmd.S = "1010";
		cmd.wr = "1";
		cmd.v = "0000";
		cmd.addr_wr = to_wr.to_string();

		vector<command> _const = cmd_const(words[1]);
		return cmd_merge(_const, cmd);
	}
	else if (words[1] == "in") {
		command cmd;
		cmd.S = "1111";
		cmd.A = "1";
		cmd.wr = "1";
		cmd.v = "0001";
		cmd.addr_wr = to_wr.to_string();

		vector<string> result;
		result.push_back(cmd.result());
		return result;
	}
	else {
		command cmd;
		cmd.S = "1111";
		cmd.wr = "1";
		cmd.v = "0001";
		cmd.addr_wr = to_wr.to_string();

		vector<string> result;
		result.push_back(cmd.result());
		return result;
	}
}
vector<string> cmd_out(vector<string>& words) {
	current_pos++;

	if (words.size() != 3) return vector<string>();
	if (words[2] == "in" || words[2][0] == '!') return vector<string>();

	bitset<2> to_out = stoi(words[2]);
	command cmd;

	if (words[1] == "in") {
		cmd.A = "1";
		cmd.v = "1001";
		cmd.addr_wr = "00" + to_out.to_string();

		vector<string> result;
		result.push_back(cmd.result());
		return result;
	}
	else if (words[1][0] == '!') {
		cmd.S = "0101";
		cmd.v = "1000";
		cmd.addr_wr = "00" + to_out.to_string();

		vector<command> _const = cmd_const(words[1]);
		return cmd_merge(_const, cmd);
	}
	else {
		cmd.v = "1001";
		cmd.addr_wr = "00" + to_out.to_string();

		vector<string> result;
		result.push_back(cmd.result());
		return result;
	}
}
vector<string> cmd_lbl(vector<string>& words) {
	if (words.size() != 2) return vector<string>();
	labels[words[1]] = current_pos + 1;
	current_pos++;
	vector<string> result;
	result.push_back("\n");
	return result;
}

int error(ifstream& fin, ofstream& fout) {
	fout << "Error" << endl;
	fin.close();
	fout.close();
	return -1;
}

int main(int argc, char** argv) {
	if (argc != 2) return -1;

	commands["nop"] = cmd_nop;
	commands["jne"] = cmd_jne;
	commands["jg"] = cmd_jg;
	commands["jl"] = cmd_jl;
	commands["je"] = cmd_je;
	commands["jge"] = cmd_jge;
	commands["jle"] = cmd_jle;
	commands["jmp"] = cmd_jmp;
	commands["mov"] = cmd_mov;
	commands["add"] = cmd_add;
	commands["sub"] = cmd_sub;
	commands["shr"] = cmd_shr;
	commands["shl"] = cmd_shl;
	//commands["inc"] = cmd_inc;
	//commands["dec"] = cmd_dec;
	commands["and"] = cmd_and;
	//commands["or"] = cmd_or;
	//commands["xor"] = cmd_xor;
	commands["not"] = cmd_not;
	commands["out"] = cmd_out;
	commands["lbl"] = cmd_lbl;

	ifstream fin(argv[1], ifstream::binary);
	ofstream fout(string("_") + argv[1], ofstream::binary | ofstream::trunc);

	vector<string> program;

	while (true) {
		char buffer[128];

		fin.getline(buffer, 128);
		if (fin.eof()) break;
		if (buffer[0] == '\r') continue;

		string line(buffer);
		vector<string> command = break_word(line);
		vector<string> (*func)(vector<string>&) = commands[command[0]];
		if (func == 0) return error(fin, fout);
		
		vector<string> cmd = func(command);
		if (cmd.size() == 0) return error(fin, fout);

		program.insert(program.end(), cmd.begin(), cmd.end());
	}

	for (auto const& kvp : jmps) {
		int pos = kvp.first;
		string label = kvp.second;
		int dest = labels[label];
		if (dest == 0) {
			fout << "Label '" << label << "' not found" << endl;
			return error(fin, fout);
		}
		dest--;
		bitset<8> _dest = dest;

		program[pos] += _dest.to_string();
	}
	for (string line : program) fout << line << endl;

	fin.close();
	fout.close();

	cout << current_pos << endl;

	return 0;
}
