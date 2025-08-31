#include <cctype>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <filesystem>

enum tokens {
    RETURN,
    INT_LIT,
    SEMI,
    INVALID,
    EQUAL,
    STR_LIT,
    LET,
    PRINT,
    L_PAREN,
    R_PAREN,
    IDENT
};

class Token {
  public:
    tokens type;
    int value;
    std::string val;

    Token(const tokens& n) : type(n), value(0), val("") {}
    Token(const tokens& n, int a) : type(n), value(a), val("") {}
    Token(const tokens& n, const std::string& s) : type(n), value(0), val(s) {}
};

static std::string sysVars;
static int sysVarC = 0;
static std::unordered_map<std::string, size_t> varLengths;

static std::string escape_nasm(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;

            default:
                if (c >= 32 && c <= 126) out.push_back(c);
                else {
                    // non-printable: use \xHH
                    const char hex[] = "0123456789ABCDEF";
                    out += "\\x";
                    out.push_back(hex[(c >> 4) & 0xF]);
                    out.push_back(hex[c & 0xF]);
                }
        }
    }
    return out;
}

static std::string addSysVar(const std::string &s) {
    std::string name = "sysvar" + std::to_string(sysVarC++);
    std::string escaped = escape_nasm(s);
    sysVars += name + " db \"" + escaped + "\", 0\n";
    return name;
}

std::vector<Token> getTokens(const std::string &str) {
    std::vector<Token> token_list;
    const size_t n = str.length();

    for (size_t i = 0; i < n; ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);

        // identifier (letters/digits, starting with letter)
        if (std::isalpha(c)) {
            std::string buf;
            size_t j = i;
            while (j < n && std::isalnum(static_cast<unsigned char>(str[j]))) {
                buf += str[j++];
            }
            if (buf == "return") token_list.emplace_back(RETURN);
            else if (buf == "let") token_list.emplace_back(LET);
            else if (buf == "print") token_list.emplace_back(PRINT);
            else token_list.emplace_back(IDENT, buf);
            i = j - 1;
            continue;
        }

        // integer literal
        if (std::isdigit(c)) {
            std::string buf;
            size_t j = i;
            while (j < n && std::isdigit(static_cast<unsigned char>(str[j]))) {
                buf += str[j++];
            }
            token_list.emplace_back(INT_LIT, std::stoi(buf));
            i = j - 1;
            continue;
        }

        // quoted string literal (with basic escapes)
        if (str[i] == '"') {
            std::string buf;
            size_t j = i + 1;
            while (j < n) {
                char ch = str[j++];
                if (ch == '\\' && j < n) {
                    char esc = str[j++];
                    switch (esc) {
                        case 'n': buf.push_back('\n'); break;
                        case 't': buf.push_back('\t'); break;
                        case '\\': buf.push_back('\\'); break;
                        case '"': buf.push_back('"'); break;
                        default:
                            // unknown escape: keep both characters
                            buf.push_back('\\');
                            buf.push_back(esc);
                            break;
                    }
                } else if (ch == '"') {
                    // closing quote
                    break;
                } else {
                    buf.push_back(ch);
                }
            }
            token_list.emplace_back(STR_LIT, buf);
            i = j - 1;
            continue;
        }

        // single-character tokens
        if (str[i] == ';') { token_list.emplace_back(SEMI); continue; }
        if (str[i] == '=') { token_list.emplace_back(EQUAL); continue; }
        if (str[i] == '(') { token_list.emplace_back(L_PAREN); continue; }
        if (str[i] == ')') { token_list.emplace_back(R_PAREN); continue; }

        if (std::isspace(c)) continue;

        token_list.emplace_back(INVALID);
        std::cerr << "FOUND AN INVALID at i = " << i << " char = '" << str[i] << "'\n";
        break;
    }

    return token_list;
}

std::string variables(const std::vector<Token> &t) {
    std::string code;
    for (int i = 0; i < (int)t.size(); ++i) {
        if (t[i].type == LET) {
            // let ident = int_lit ;
            if (i + 4 < (int)t.size()) {
                if (t[i+1].type == IDENT && t[i+2].type == EQUAL && t[i+3].type == INT_LIT && t[i+4].type == SEMI) {
                    code += (t[i+1].val + " dd " + std::to_string(t[i+3].value) + "\n");
                    i += 4;
                    continue;
                }
            }
            // let ident = "string";
            if (i + 4 < (int)t.size()) {
                if (t[i+1].type == IDENT && t[i+2].type == EQUAL && t[i+3].type == STR_LIT && t[i+4].type == SEMI) {
                    std::string escaped = escape_nasm(t[i+3].val);
                    code += (t[i+1].val + " db \"" + escaped + "\", 0\n");
                    varLengths[t[i+1].val] = t[i+3].val.length();
                    i += 4;
                    continue;
                }
            }
        }
    }
    return code;
}

std::string codegen(const std::vector<Token> &t) {
    std::string code;
    for (int i = 0; i < (int)t.size(); ++i) {
        if (t[i].type == RETURN) {
            // return int_literal ;
            if (i + 2 < (int)t.size()) {
                if (t[i+1].type == INT_LIT && t[i+2].type == SEMI) {
                    code += "    mov rax, 60\n";
                    code += "    mov rdi, " + std::to_string(t[i+1].value) + "\n";
                    code += "    syscall\n\n";
                    i += 2;
                    continue;
                }
                // return ident ;
                if (t[i+1].type == IDENT && t[i+2].type == SEMI) {
                    // read 32-bit dd into edi
                    code += "    mov rax, 60\n";
                    code += "    mov edi, dword [rel " + t[i+1].val + "]\n";
                    code += "    syscall\n\n";
                    i += 2;
                    continue;
                }
            }
            std::cerr << "return used without a valid return value\n";
            break;
        }

        if (t[i].type == PRINT) {
            // print("literal");
            if (i + 4 < (int)t.size()) {
                if (t[i+1].type == L_PAREN && t[i+2].type == STR_LIT && t[i+3].type == R_PAREN && t[i+4].type == SEMI) {
                    std::string name = addSysVar(t[i+2].val);
                    code += "    mov rax, 1\n";
                    code += "    mov rdi, 1\n";
                    code += "    lea rsi, [rel " + name + "]\n";
                    code += "    mov rdx, " + std::to_string(t[i+2].val.length()) + "\n";
                    code += "    syscall\n\n";
                    i += 4;
                    continue;
                }
            }
            // print(var);
            if (i + 4 < (int)t.size()) {
                if (t[i+1].type == L_PAREN && t[i+2].type == IDENT && t[i+3].type == R_PAREN && t[i+4].type == SEMI) {
                    std::string varName = t[i+2].val;
                    size_t len = 0;
                    auto it = varLengths.find(varName);
                    if (it != varLengths.end()) len = it->second;
                    code += "    mov rax, 1\n";
                    code += "    mov rdi, 1\n";
                    code += "    lea rsi, [rel " + varName + "]\n";
                    code += "    mov rdx, " + std::to_string(len) + "\n";
                    code += "    syscall\n\n";
                    i += 4;
                    continue;
                }
            }

            std::cerr << "unrecognized print pattern at token index " << i << std::endl;
        }
    }

    code += "    mov rax, 60\n";
    code += "    xor rdi, rdi\n";
    code += "    syscall\n";

    return code;
}

std::string usingTokens(const std::vector<Token> &t) {
    std::string data_section = variables(t);
    std::string text_code = codegen(t);

    std::string asmCode;
    asmCode += "BITS 64\n\n";
    asmCode += "section .data\n";
    asmCode += data_section;
    if (!sysVars.empty()) {
        asmCode += "\n";
        asmCode += sysVars;
    }
    asmCode += "\nsection .text\n";
    asmCode += "global _start\n\n";
    asmCode += "_start:\n";
    asmCode += text_code;

    return asmCode;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <sourcefile>\n";
        return 1;
    }

    std::filesystem::create_directories("./asm");

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Failed to open source file: " << argv[1] << std::endl;
        return 1;
    }
    std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto found = getTokens(code);

    for (int i = 0; i < (int)found.size(); ++i) {
        std::cout << "tok[" << i << "] type=" << found[i].type
                  << " val=\"" << found[i].val << "\""
                  << " int=" << found[i].value << "\n";
    }

    std::string asmText = usingTokens(found);

    std::ofstream asmFile("./asm/main1.asm");
    if (!asmFile) {
        std::cerr << "Failed to write ./asm/main1.asm\n";
        return 1;
    }
    asmFile << asmText;
    asmFile.close();

    std::cout << "Wrote ./asm/main1.asm\n";
    return 0;
}

