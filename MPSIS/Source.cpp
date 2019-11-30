#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <bitset>

using namespace std;

/* COMMAND STRUCT - total 29b

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

[9-15]
2b - ISR, ISL
1b - A
1b - wr
3b - v

[16-28]
1b - true DataIn or const
4b - DataIn
4b - addr1
4b - addr2

COMMAND EXAMPLES:

jmp !4		- jmp at absolute cmd
je -2		- jmp at relative cmd

mov !14 3	- move constant "14" to addr 3
mov 3 4		- move addr 3 to addr 4

add 3 4	5	- add addr 4 to add 3 save to addr 5
└---┬---- ld 3		- load addr 3 to RegA
	└---- add 4 5	- add RegA to addr 4 save to addr 5
add 3 !2 4	- add constant "2" to addr 3 save to addr 4

sub 5 3	5	- sub addr 3 from addr 5 save to addr 5
└---┬---- ld 5		- load addr 5 to RegA
	└---- sub 3 5	- sub addr 3 to RegA save to addr 5
sub 5 !3 1	- sub constant "3" from addr 5 save to addr 1

inc 3 4		- increment addr 3 save to addr 4
dec 4 3		- decrement addr 4 save to addr 3

shr 5 1 2	- shift right addr 5 with 1 save to addr 2
shl 2 0 2	- shift left addr 2 with 0 save to addr 2

COMMAND RULES:

* Symbol '!' mean constant, else mean addr
* Keyword 'in' mean true DataIn
* Don't use addr 0
* Jump: 0..4096

*/

map<string, string(*)(vector<string>&)> commands;

vector<string> break_word(string& line) {
	vector<string> words;
	int space_pos = 0;
	int prev_pos = 0;
	while ((space_pos = line.find(' ', prev_pos)) != string::npos) {
		words.push_back(line.substr(prev_pos, space_pos - prev_pos));
		prev_pos = space_pos + 1;
	}
	string last = line.substr(prev_pos);
	words.push_back(last.substr(0, last.length() - 1));
	return words;
}

string jmp_body(string code, string& dest) {
	bitset<12> pc = atoi(dest.c_str() + 1) + 1;
	if (pc.to_ulong() > 256) return "";
	return code + "00000000000000" + pc.to_string();
}
string cmd_nop(vector<string>& words) {
	return "00000000000000000000000000000";
}
string cmd_jne(vector<string>& words) {
	if (words.size() != 2) return "";
	return jmp_body("001", words[1]);
}
string cmd_jg(vector<string>& words) {
	if (words.size() != 2) return "";
	return jmp_body("010", words[1]);
}
string cmd_jl(vector<string>& words) {
	if (words.size() != 2) return "";
	return jmp_body("011", words[1]);
}
string cmd_je(vector<string>& words) {
	if (words.size() != 2) return "";
	return jmp_body("100", words[1]);
}
string cmd_jge(vector<string>& words) {
	if (words.size() != 2) return "";
	return jmp_body("101", words[1]);
}
string cmd_jle(vector<string>& words) {
	if (words.size() != 2) return "";
	return jmp_body("110", words[1]);
}
string cmd_jmp(vector<string>& words) {
	if (words.size() != 2) return "";
	return jmp_body("111", words[1]);
}
string cmd_mov(vector<string>& words) {
	if (words.size() != 3) return "";
	if (words[2][0] == '!' || words[2] == "in") return "";
	string result = "00000000000110010";
	bitset<4> in, one, two;
	if (words[1][0] == '!') {
		in = atoi(words[1].c_str() + 1);
		one = 0;
	}
	else if (words[1] == "in") {
		result[16] = '1';
		in = 0;
		one = 0;
	}
	else {
		result[11] = '0';
		in = 0;
		one = stoi(words[1]);
		if (one == 0) return "";
	}
	two = stoi(words[2]);
	if (two == 0 && words[0] != "") return "";
	return result + in.to_string() + one.to_string() + two.to_string();
}
string cmd_lda(string& addr) {
	bitset<4> bitaddr = stoi(addr);
	return "000000000000000100000" + bitaddr.to_string() + "0000";
}
string cmd_ldb(string& addr) {
	bitset<4> bitaddr = stoi(addr);
	return "000000000000011000000" + bitaddr.to_string() + "0000";
}
string cmd_add(vector<string>& words) {
	if (words.size() != 4) return "";
	if (words[3][0] == '!' || words[3] == "in") return "";
	string result = "00010011000111110";
	bitset<4> in, one, two;
	if (words[1][0] == '!') {
		if (words[2][0] == '!') {
			// Const + Const -> mov then add
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[2].c_str() + 1);
			one = 0;
		}
		else if (words[2] == "in") {
			// Const + DataIn -> mov then add
			result[16] = '1';
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else {
			// Const + Addr -> add
			in = atoi(words[1].c_str() + 1);
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	else if (words[1] == "in") {
		if (words[2][0] == '!') {
			// DataIn + Const -> mov then add
			result[16] = '1';
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[1].c_str() + 1);
			one = 0;
		}
		else if (words[2] == "in") {
			// DataIn + DataIn -> mov then add
			result[16] = '1';
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else {
			// DataIn + Addr -> add
			result[16] = '1';
			in = 0;
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	else {
		if (words[2][0] == '!') {
			// Addr + Const -> add
			in = atoi(words[2].c_str() + 1);
			one = stoi(words[1]);
			if (one == 0) return "";
		}
		else if (words[2] == "in") {
			// Addr + DataIn -> add
			result[16] = '1';
			in = 0;
			one = stoi(words[1]);
			if (one == 0) return "";
		}
		else {
			// Addr + Addr -> ld then add
			result[11] = '0';
			result[15] = '0';
			result = cmd_lda(words[1]) + "\r\n" + result;
			in = 0;
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	two = stoi(words[3]);
	if (two == 0) return "";
	return result + in.to_string() + one.to_string() + two.to_string();
}
string cmd_sub(vector<string>& words) {
	if (words.size() != 4) return "";
	if (words[3][0] == '!' || words[3] == "in") return "";
	string result = "00001101100111110";
	bitset<4> in, one, two;
	if (words[1][0] == '!') {
		if (words[2][0] == '!') {
			// Const + Const -> mov then sub
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[2].c_str() + 1);
			one = 0;
		}
		else if (words[2] == "in") {
			// Const + DataIn -> mov then sub
			result[16] = '1';
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else {
			// Const + Addr -> sub
			in = atoi(words[1].c_str() + 1);
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	else if (words[1] == "in") {
		if (words[2][0] == '!') {
			// DataIn + Const -> mov then sub
			result[16] = '1';
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[1].c_str() + 1);
			one = 0;
		}
		else if (words[2] == "in") {
			// DataIn + DataIn -> mov then sub
			result[16] = '1';
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else {
			// DataIn + Addr -> sub
			result[16] = '1';
			in = 0;
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	else {
		if (words[2][0] == '!') {
			// Addr + Const -> sub
			in = atoi(words[2].c_str() + 1);
			one = stoi(words[1]);
			if (one == 0) return "";
		}
		else if (words[2] == "in") {
			// Addr + DataIn -> sub
			result[16] = '1';
			in = 0;
			one = stoi(words[1]);
			if (one == 0) return "";
		}
		else {
			// Addr + Addr -> ld then sub
			result[11] = '0';
			result[15] = '0';
			result = cmd_lda(words[1]) + "\r\n" + result;
			in = 0;
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	two = stoi(words[3]);
	if (two == 0) return "";
	return result + in.to_string() + one.to_string() + two.to_string();
}
string cmd_shr(vector<string>& words) {
	if (words.size() != 4) return "";
	if (words[1][0] == '!' || words[3][0] == '!' || words[1] == "in" || words[3] == "in") return "";
	string result = "00001010000011000";
	bitset<4> in, one, two;
	if (words[2] == "1") result[9] = '1';
	else if (words[2] != "0") return "";
	in = 0;
	one = 0;
	two = stoi(words[3]);
	if (two == 0) return "";
	return cmd_ldb(words[1]) + "\r\n" + result + in.to_string() + one.to_string() + two.to_string();
}
string cmd_shl(vector<string>& words) {
	if (words.size() != 4) return "";
	if (words[1][0] == '!' || words[3][0] == '!' || words[1] == "in" || words[3] == "in") return "";
	string result = "00001010000010100";
	bitset<4> in, one, two;
	if (words[2] == "1") result[9] = '1';
	else if (words[2] != "0") return "";
	in = 0;
	one = 0;
	two = stoi(words[3]);
	if (two == 0) return "";
	return cmd_ldb(words[1]) + "\r\n" + result + in.to_string() + one.to_string() + two.to_string();
}
//string cmd_inc(vector<string>& words) {
//	if (line[3] != ' ') return "";
//	string result = "000";
//	return result;
//}
//string cmd_dec(vector<string>& words) {
//	if (line[3] != ' ') return "";
//	string result = "000";
//	return result;
//}
string cmd_and(vector<string>& words) {
	if (words.size() != 4) return "";
	string result = "00001000000111110";
	bitset<4> in, one, two;
	if (words[1][0] == '!') {
		if (words[1][0] == '!') {
			// Const & Const -> mov then and
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[2].c_str() + 1);
			one = 0;
		}
		else if (words[1] == "in") {
			// Const & DataIn -> mov then and
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[1].c_str() + 1);
			one = 0;
		}
		else {
			// Const & Addr -> and
			in = atoi(words[1].c_str() + 1);
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	else if (words[1] == "in") {
		if (words[1][0] == '!') {
			// DataIn & Const -> mov then and
			result[16] = '1';
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else if (words[1] == "in") {
			// DataIn & DataIn -> mov then and
			result[16] = '1';
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else {
			// DataIn & Addr -> and
			result[16] = '1';
			in = 0;
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	else {
		if (words[1][0] == '!') {
			// Addr & Const -> and
			in = atoi(words[2].c_str() + 1);
			one = stoi(words[1]);
			if (one == 0) return "";
		}
		else if (words[1] == "in") {
			// Addr & DataIn -> and
			result[16] = '1';
			in = 0;
			one = stoi(words[1]);
			if (one == 0) return "";
		}
		else {
			// Addr & Addr -> ld then and
			if (stoi(words[2]) == 0) return "";
			result[11] = '0';
			result[15] = '0';
			result = cmd_lda(words[2]) + "\r\n" + result;
			in = 0;
			one = stoi(words[1]);
			if (one == 0) return "";
		}
	}
	two = stoi(words[3]);
	if (two == 0) return "";
	return result + in.to_string() + one.to_string() + two.to_string();
}
string cmd_or(vector<string>& words) {
	if (words.size() != 4) return "";
	string result = "00000010000111110";
	bitset<4> in, one, two;
	if (words[1][0] == '!') {
		if (words[1][0] == '!') {
			// Const & Const -> mov then or
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[2].c_str() + 1);
			one = 0;
		}
		else if (words[1] == "in") {
			// Const & DataIn -> mov then or
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[1].c_str() + 1);
			one = 0;
		}
		else {
			// Const & Addr -> or
			in = atoi(words[1].c_str() + 1);
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	else if (words[1] == "in") {
		if (words[1][0] == '!') {
			// DataIn & Const -> mov then or
			result[16] = '1';
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else if (words[1] == "in") {
			// DataIn & DataIn -> mov then or
			result[16] = '1';
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else {
			// DataIn & Addr -> or
			result[16] = '1';
			in = 0;
			one = stoi(words[2]);
		}
	}
	else {
		if (words[1][0] == '!') {
			// Addr & Const -> or
			in = atoi(words[2].c_str() + 1);
			one = stoi(words[2]);
			if (one == 0) return "";
		}
		else if (words[1] == "in") {
			// Addr & DataIn -> or
			result[16] = '1';
			one = stoi(words[2]);
			if (one == 0) return "";
		}
		else {
			// Addr & Addr -> ld then or
			result[11] = '0';
			result[15] = '0';
			if (stoi(words[2]) == 0) return "";
			result = cmd_lda(words[2]) + "\r\n" + result;
			in = 0;
			one = stoi(words[1]);
			if (one == 0) return "";
		}
	}
	two = stoi(words[3]);
	if (two == 0) return "";
	return result + in.to_string() + one.to_string() + two.to_string();
}
string cmd_xor(vector<string>& words) {
	if (words.size() != 4) return "";
	string result = "00010010000111110";
	bitset<4> in, one, two;
	if (words[1][0] == '!') {
		if (words[1][0] == '!') {
			// Const & Const -> mov then xor
			vector<string> buff = { "", words[1], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[2].c_str() + 1);
			one = 0;
		}
		else if (words[1] == "in") {
			// Const & DataIn -> mov then xor
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = atoi(words[1].c_str() + 1);
			one = 0;
		}
		else {
			// Const & Addr -> xor
			in = atoi(words[1].c_str() + 1);
			one = stoi(words[2]);
			if (one == 0) return "";
		}
	}
	else if (words[1] == "in") {
		if (words[1][0] == '!') {
			// DataIn & Const -> mov then xor
			result[16] = '1';
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else if (words[1] == "in") {
			// DataIn & DataIn -> mov then xor
			result[16] = '1';
			vector<string> buff = { "", words[2], "0" };
			result = cmd_mov(buff) + "\r\n" + result;
			in = 0;
			one = 0;
		}
		else {
			// DataIn & Addr -> xor
			result[16] = '1';
			in = 0;
			one = stoi(words[2]);
		}
	}
	else {
		if (words[1][0] == '!') {
			// Addr & Const -> xor
			in = atoi(words[2].c_str() + 1);
			one = stoi(words[2]);
			if (one == 0) return "";
		}
		else if (words[1] == "in") {
			// Addr & DataIn -> xor
			result[16] = '1';
			one = stoi(words[2]);
			if (one == 0) return "";
		}
		else {
			// Addr & Addr -> ld then xor
			result[11] = '0';
			result[15] = '0';
			if (stoi(words[2]) == 0) return "";
			result = cmd_lda(words[2]) + "\r\n" + result;
			in = 0;
			one = stoi(words[1]);
			if (one == 0) return "";
		}
	}
	two = stoi(words[3]);
	if (two == 0) return "";
	return result + in.to_string() + one.to_string() + two.to_string();
}
string cmd_not(vector<string>& words) {
	if (words.size() != 3) return "";
	if (words[2][0] == '!' || words[2] == "in") return "";
	string result = "00011110000110010";
	bitset<4> in, one, two;
	if (words[1][0] == '!') {
		in = atoi(words[1].c_str() + 1);
		one = 0;
	}
	else if (words[1] == "in") {
		result[16] = '1';
		in = 0;
		one = 0;
	}
	else {
		result[11] = '0';
		in = 0;
		one = stoi(words[1]);
		if (one == 0) return "";
	}
	two = stoi(words[2]);
	return result + in.to_string() + one.to_string() + two.to_string();
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
	commands["or"] = cmd_or;
	commands["xor"] = cmd_xor;
	commands["not"] = cmd_not;

	ifstream fin(argv[1], ifstream::binary);
	ofstream fout(string("_") + argv[1], ofstream::binary | ofstream::trunc);

	vector<string> buff;
	fout << cmd_nop(buff) << endl;

	while (true) {
		char buffer[128];

		fin.getline(buffer, 128);
		if (fin.eof()) break;

		string line(buffer);
		vector<string> command = break_word(line);
		string (*func)(vector<string>&) = commands[command[0]];
		if (func == 0) return error(fin, fout);
		
		string cmd = func(command);
		if (cmd == "") return error(fin, fout);

		fout << cmd << endl;
	}

	fin.close();
	fout.close();

	return 0;
}
