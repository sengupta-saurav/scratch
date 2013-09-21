/*
 * Push-down automaton for interpreting postfix arithmetic expressions
 * Saurav Sengupta (sengupta.saurav.01@gmail.com)
 * 2010
 */
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <list>
#include <cstring>
using namespace std;

// Various markers and operators
const wchar_t EOI = L';';
const wchar_t ZERO = L'0';
const wchar_t MINUS = L'-';
const wchar_t PLUS = L'+';
const wchar_t MULT = L'*';
const wchar_t DIV = L'/';
const wchar_t RADIX_POINT = L'.';

bool EndOfInput = false;	// whether we have reached end of input
bool IsOperator = false;	// whether we have got an operator

// Exception for invalid expressions
class InvalidExpression : public runtime_error {
	public:
		InvalidExpression(const wstring& str) : 
			runtime_error("Invalid expression"), expr(str) {}
			
		InvalidExpression(const InvalidExpression& e) : 
			runtime_error(e), expr(e.expr) {}
			
		~InvalidExpression() throw() {}
			
		const wchar_t* expression() const { return expr.c_str(); }
			
	private:
		wstring expr;
};

// Exception to be used if entire input is invalid
class InvalidInput : public runtime_error {
	public:
		InvalidInput(const wstring& str = L"") : 
			runtime_error("Invalid input"), expr(str) {}
			
		InvalidInput(const InvalidInput& e) : 
			runtime_error(e), expr(e.expr) {}
			
		~InvalidInput() throw() {}
			
		const wchar_t* expression() const { return expr.c_str(); }
			
	private:
		wstring expr;
};

// Exception for division by zero
template <class T> 
class DivByZero : public runtime_error {
	public:
		DivByZero(T n) : 
			runtime_error("Division by zero"), opd(n) {}
			
		DivByZero(const DivByZero& e) : 
			runtime_error(e), opd(e.opd) {}
			
		~DivByZero() throw() {}
			
		const T operand() const { return opd; }
			
	private:
		T opd;
};

// Returns the next token found in the given input stream
wstring next_tok(wistream& is) {
	wstring str;				// the token
	wchar_t c = L'\0';			// character to be read from the input
	bool got_radixpt = false;	// whether we have got a radix point
	bool is_valid_num = false;	// whether the number read is valid
	bool should_cont = true;	// whether reading from input should continue
	bool got_eoi = false;		// whether we have reached end of input
	
	while (should_cont && is.get(c)) {
		switch (c) {
			case EOI:
				got_eoi = true;
				should_cont = false;
				break;
				
			case MINUS: case PLUS:
				if (!str.empty()) {	// already have something; don't need this
					is.putback(c);
					should_cont = false;
					break;
				}
				str += c;
				break;
				
			case RADIX_POINT:
				if (got_radixpt) {
					is.putback(c);
					should_cont = false;
					break;
				}
				if (str.empty()) str += ZERO;	// for cases like .2
				str += c;
				got_radixpt = true;
				is_valid_num = false;	// no. can't end with radix point
				break;
				
			case MULT: case DIV:
				if (!str.empty()) is.putback(c);
				else str += c;
				should_cont = false;
				break;
				
			default:
				if (isspace(c, is.getloc())) {
					if (str.empty()) break;
					else {	// a space is a delimiter
						is.putback(c);
						should_cont = false;
						break;
					}
				}
				if (!isdigit(c, is.getloc())) {	// invalid expression
					str += c;	// store it to show the user
					is.putback(c);
					should_cont = false;
					is_valid_num = false;
					break;
				}
				str += c;
				is_valid_num = true;	// got a digit
		}
	}
	
	if (!is.good() && !is.eof())
		throw ios_base::failure("Input stream failure");
	
	// The weird string constructor is a C++ STL oddity - you can't 
	// contruct a string from a single character without specifying 
	// how many times you want to repeat the character!
	IsOperator = (str == wstring(1, MINUS) || str == wstring(1, PLUS) || 
				  str == wstring(1, MULT) || str == wstring(1, DIV));
	
	if (is.eof() || got_eoi) EndOfInput = true;
	if (!EndOfInput && !is_valid_num && !IsOperator)
		throw InvalidExpression(str);
	
	return str;
}

// Prints the contents of the PDA stack using STL iterators
template <class T> 
void print_stack(T si, T si_end) {
	while (si != si_end) {
		wcout << *si << L" ";
		si++;
	}
}

// Main working: If we get a number, push it onto the stack, if we get an 
// operator, pop the last two numbers off the stack, apply the operator 
// on them, then push the result back onto the stack.
int main(int argc, char* argv[]) {
	bool flag_verbose = false;	// whether to print working steps
	if (argc > 1) {
		flag_verbose = !strcasecmp(argv[1], "-v");
	}
	
	try {
		list<double> mem;	// the memory stack
		
		while (!EndOfInput) {
			wstring tok = next_tok(wcin);
			if (!EndOfInput || !tok.empty()) {
				if (!IsOperator) {
					wistringstream ss;
					ss.imbue(wcin.getloc());
					ss.str(tok);
					double n = 0.0;
					ss >> n;
					if (!ss.good() && !ss.eof())
						throw runtime_error("Could not convert to number");
					if (flag_verbose) wcout << L"Number " << n << endl;
					mem.push_back(n);
				}
				else {
					if (flag_verbose) {
						wcout << L"Operator " << tok << endl;
						wcout << L"Stack: ";
						print_stack(mem.begin(), mem.end());
					}
					
					// The number at the top of the stack is actually the 
					// second operand; the one below it is the first operand
					if (mem.empty()) throw InvalidInput();
					double n2 = mem.back(); mem.pop_back();
					if (mem.empty()) throw InvalidInput();
					double n1 = mem.back(); mem.pop_back();
					if (tok == wstring(1, DIV) && n2 == 0.0) {
						if (flag_verbose) wcout << endl;
						throw DivByZero<double>(n1);
					}
					
					double res = 0.0;
					
					if (tok == wstring(1, PLUS)) res = n1 + n2;
					if (tok == wstring(1, MINUS)) res = n1 - n2;
					if (tok == wstring(1, MULT)) res = n1 * n2;
					if (tok == wstring(1, DIV)) res = n1 / n2;
					
					if (flag_verbose) {
						wcout << L"\n" << n1 << L" " << tok << L" " << n2 
							  << L" = " << res << endl;
					}
					
					mem.push_back(res);
					
					if (flag_verbose) {
						wcout << L"Stack: ";
						print_stack(mem.begin(), mem.end());
						wcout << L"\n" << endl;
					}
				}
			}
		}
		
		// Print the final result
		// The stack should be empty after popping this result if 
		// the input was a correct and complete postfix expression
		if (flag_verbose) wcout << L"Result: ";
		wcout << mem.back() << endl;
		mem.pop_back();
		if (!mem.empty()) {
			wcerr << L"The input was improper; the stack is not empty.";
			if (flag_verbose) {
				wcerr << L"\nStack: ";
				print_stack(mem.begin(), mem.end());
			}
			wcerr << endl;
		}
	}
	catch (InvalidExpression& e) {
		wcerr << L"Invalid expression: " << e.expression() << endl;
	}
	catch (InvalidInput& e) {
		wcerr << L"Invalid input";
		if (wcslen(e.expression()) > 0) wcerr << L": " << e.expression();
		wcerr << endl;
	}
	catch (DivByZero<double>& e) {
		wcerr << L"Division by zero: " << e.operand() << L" / 0" << endl;
	}
	catch (ios_base::failure& e) {
		cerr << e.what() << endl;
	}
	catch (runtime_error& e) {
		cerr << e.what() << endl;
	}
	
	return 0;
}
