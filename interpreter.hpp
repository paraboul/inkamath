#ifndef H_PARSER
#define H_PARSER

#include <iostream>
#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <utility>
#include <cctype> // isalpha
#include <map>
#include <stdexcept>

#include "expression.hpp"
#include "matrix.hpp"
#include "token.hpp"
#include "workspace_manager.hpp"
#include "numeric_interface.hpp"

#define PExpression std::shared_ptr<Expression<U> >

template <typename T, typename U=Matrix<T> >
class Interpreter
{
public:
    Interpreter();
    ~Interpreter();

    typedef typename U::value_type value_type;
    typedef U matrix_type;

    U Eval(const std::string& s);
    void PrintTokens(void);

    void ResetInterpreter(void);

private:
    void Lexer(const std::string& s);
    void Number_Lexer(const std::string& s, size_t& i);
    void Reference_Lexer(const std::string& s, size_t& i);

    PExpression ParseAll();
    PExpression Parse();
    PExpression ParseEqualExpr();
    PExpression ParseAddExpr();
    PExpression ParseMultExpr();
    PExpression ParsePowExpr();
    PExpression ParseMatrix();
    PExpression ParseSimpleExpr();
    PExpression ParseParameters();
    PExpression ParseSubExpr();

    std::list< Token<T> > m_toklist;
    typename std::list< Token<T> >::iterator m_i;

    PExpression m_E;
    std::ostringstream oss;
};

template <typename T, typename U>
Interpreter<T,U>::Interpreter()
{}

template <typename T, typename U>
Interpreter<T,U>::~Interpreter()
{

}

template <typename T, typename U>
void Interpreter<T,U>::ResetInterpreter()
{
    m_E.reset();
    m_toklist.clear();
    oss.str("");
    m_i = m_toklist.begin();
}

template <typename T, typename U>
void Interpreter<T,U>::Lexer(const std::string& s)
{
    size_t i = 0;
    for (i=0; i < s.length(); i++)
    {
        switch (s[i])
        {
        case '(':
            m_toklist.push_back(Token<T>(LPar));
            break;
        case ')':
            m_toklist.push_back(Token<T>(RPar));
            break;
        case '[':
            m_toklist.push_back(Token<T>(LBra));
            break;
        case ']':
            m_toklist.push_back(Token<T>(RBra));
            break;
        case ',':
            m_toklist.push_back(Token<T>(Comma));
            break;
        case ';':
            m_toklist.push_back(Token<T>(Semico));
            break;
        case '+':
            m_toklist.push_back(Token<T>(Add));
            break;
        case '-':
            m_toklist.push_back(Token<T>(Min));
            break;
        case '*':
            m_toklist.push_back(Token<T>(Mult));
            break;
        case '=':
            m_toklist.push_back(Token<T>(Equal));
            break;
        case '/':
            m_toklist.push_back(Token<T>(Div));
            break;
        case '^':
            m_toklist.push_back(Token<T>(Pow));
            break;
        case '!':
            m_toklist.push_back(Token<T>(Fact));
            break;
        case '_':
            m_toklist.push_back(Token<T>(Sub));
            break;
        case ' ':
            break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'i':
            this->Number_Lexer(s,i);
            break;
		default:
            if(std::isalpha(s[i]))
                    Reference_Lexer(s,i);
            else
            {
                oss <<"Unexpected character : " << s[i] << std::endl;
                throw(std::runtime_error(oss.str()));
            }
        }
    }

    if (m_toklist.empty()) throw(std::runtime_error("Cannot evaluate an empty expression.\n"));
}

template <typename T, typename U>
void Interpreter<T,U>::Number_Lexer(const std::string& s, size_t& i)
{
    T num;
    char* end = NULL;
    if(numeric_interface<T>::parse(num,&s[i],end))
    {
        i = end - &s[0] - 1;
        m_toklist.push_back(Token<T>(Val,num));
    }
    else
    {
        throw(std::runtime_error("Failed to parse number.\n"));
    }
}

template <typename T, typename U>
void Interpreter<T,U>::Reference_Lexer(const std::string &s, size_t& i)
{
    size_t s_i = i;
    while ((i < s.length()) && isalpha(s[i]))
    {
        ++i;
    }
    if (i!=s_i)
    {
        while (i < s.length() && isdigit(s[i]))
        {
            ++i;
        }
        std::string name(s.substr(s_i,i-s_i));
        m_toklist.push_back(Token<T>(Func,0,name));
        --i;
    }
    else
    {
        oss << "Unexcepected character '" << s[i] << "'" << std::endl;
        throw(std::runtime_error(oss.str()));
    }

}



template <typename T, typename U>
PExpression Interpreter<T,U>::ParseAll()
{
    m_i = m_toklist.begin();
    PExpression e = Parse();
    if (m_i != m_toklist.end())
    {
        oss << "Syntax error before '" << m_i->Print() << "'" << std::endl;
        if (m_i->type == LPar)
        {
            oss << "The operator '*' is probably missing." << std::endl;
        }
        throw(std::runtime_error(oss.str()));
    }
    if (e == 0)
    {
        throw(std::logic_error("Unexpected error."));
    }
    return e;
}

template <typename T, typename U>
PExpression Interpreter<T,U>::Parse()
{
    if(m_i != m_toklist.end() && m_i->type == Comma)
        ++m_i;
    return ParseEqualExpr();
}

template <typename T, typename U>
PExpression Interpreter<T,U>::ParseEqualExpr()
{
    PExpression e,func,expr,sub;
    typename std::list< Token<T> >::iterator m_s = m_i;
    if (m_i != m_toklist.end() && m_i->type == Func)
    {
        std::string name = m_i++->name;
        func = ParseParameters();
        sub = ParseSubExpr();
        if (m_i != m_toklist.end() && m_i++->type == Equal)
        {
            expr = Parse();
            e.reset(new EqualExpression<U>(PExpression(new FuncExpression<U>(name,func,sub)),expr));
            WorkSpManager<U>::Get()->SetExpr(name,e);
        }
        else
        {
            m_i = m_s;
            e = ParseAddExpr();
        }
    }
    else
    {
        e = ParseAddExpr();
    }
    return e;
}

template <typename T, typename U>
PExpression Interpreter<T,U>::ParseAddExpr()
{
    PExpression e = ParseMultExpr();
    while (m_i != m_toklist.end() && (m_i->type == Add || m_i->type == Min) )
    {
        if ((m_i++)->type == Add)
        {
            e.reset(new AddExpression<U>(e,ParseMultExpr()));
        }
        else
        {
            PExpression tmp;
            tmp.reset(new NegExpression<U>(ParseMultExpr()));
            e.reset(new AddExpression<U>(e,tmp));
        }
    }
    return e;
}

template <typename T, typename U>
PExpression Interpreter<T,U>::ParseMultExpr()
{
    PExpression e = ParsePowExpr();
    while (m_i != m_toklist.end() && (m_i->type == Mult || m_i->type == Div) )
    {
        if ((m_i++)->type == Mult)
        {
            e.reset(new MultExpression<U>(e,ParsePowExpr()));
        }
        else
        {
            e.reset(new DivExpression<U>(e,ParsePowExpr()));
        }
    }
    return e;
}

template <typename T, typename U>
PExpression Interpreter<T,U>::ParsePowExpr()
{
    PExpression e = ParseSimpleExpr();
    if (m_i != m_toklist.end() && m_i->type == Pow)
    {
        ++m_i;
        e.reset(new PowExpression<U>(e,ParsePowExpr()));
    }
    return e;
}

template <typename T, typename U>
PExpression Interpreter<T,U>::ParseMatrix()
{
    std::vector<std::vector<PExpression > > mat(1, std::vector<PExpression > (0));
    PExpression e;

    while ((m_i != m_toklist.end()) && (m_i->type != RBra) && (m_i->type != RPar))
    {
        switch (m_i->type)
        {
        case Semico :
            mat.push_back(std::vector<PExpression > ());
            ++m_i;
            break;

        default:
            e = Parse();
            (mat.at(mat.size()-1)).push_back(e);
        }
    }
    /* ATTENTION :  'mat' est lib�r� dans ~MatExpression */
    e.reset(new MatExpression<U>(mat));
    return e;
}

template <typename T, typename U>
PExpression  Interpreter<T,U>::ParseSimpleExpr()
{
    PExpression e,func,sub;
    std::string name;
    if (m_i != m_toklist.end())
    {
        switch (m_i->type)
        {
        case Val:
            e.reset(new ValExpression<U>(m_i++->value));
			break;

        case Func:
            name = m_i++->name;
            func = ParseParameters();
            sub = ParseSubExpr();
            e.reset(new FuncExpression<U>(name,func,sub));
			break;

        case Min:
            ++m_i;
            e.reset(new NegExpression<U>(ParseMultExpr()));
			break;

        case Fact:
            ++m_i;
            e.reset(new FactExpression<U>(ParsePowExpr()));
			break;

        case LPar:
            ++m_i;
            e = Parse();
            if (m_i != m_toklist.end() && m_i->type == RPar)
            {
                ++m_i;
            }
            else
            {
                oss << "Missing operator ')' after '" << (--m_i)->Print() << "'" << std::endl;
                throw(std::runtime_error(oss.str()));
            }
			break;

        case LBra:
            ++m_i;
            e = ParseMatrix();
            if (m_i != m_toklist.end() && m_i->type == RBra)
            {
                ++m_i;
            }
            else
            {
                oss << "Missing operator ']' after '" << (--m_i)->Print() << "'" << std::endl;
                throw(std::runtime_error(oss.str()));
            }
			break;

        default:
        case RPar:
            oss << "Unexpected operator '" << m_i->Print() << "'" << std::endl;
            throw(std::runtime_error(oss.str()));
            break;
        }
    }
    else
    {
        oss << "Unexpected end of input before '" << (--m_i)->Print() << "'" << std::endl;
        throw(std::runtime_error(oss.str()));
    }
    return e;
}

void print(std::string s)
{
    std::cout << s;
}

template <typename T, typename U>
PExpression Interpreter<T,U>::ParseParameters()
{
    PExpression e;
    typename std::list< Token<T> >::iterator m_s = m_i;
    if (m_i != m_toklist.end() && m_i++->type == LPar && m_i != m_toklist.end() && m_i->type != RPar)
    {
        e = ParseMatrix();
        if (m_i->type != RPar)
            throw(std::logic_error("Missing ')' after function parameters."));
        ++m_i;
    }
    else
    {
        e.reset(new EmptyExpression<U>());
        m_i=m_s;
    }
    return e;
}

template <typename T, typename U>
PExpression Interpreter<T,U>::ParseSubExpr()
{
    PExpression e;
    typename std::list< Token<T> >::iterator m_s = m_i;
    if (m_i != m_toklist.end() && m_i++->type == Sub)
    {
        e = ParseSimpleExpr();
    }
    else
    {
        e.reset(new EmptyExpression<U>());
        m_i=m_s;
    }
    return e;
}

template <typename T, typename U>
U Interpreter<T,U>::Eval(const std::string& s)
{
    U ret = U(); // relatively exception safe :o
    try
    {
        /* the following functions might throw some evaluation errors */
        Lexer(s);
        m_E = ParseAll();
        ret = m_E->Eval();
    }
    catch (const std::exception& e)
    {
        std::cout << "Error : " << e.what();
    }
    catch (...)
    {
        std::cout << "Unknown error" << std::endl;
    }
    ResetInterpreter(); // reset whatever happens and forgive the user
    return ret;
}

template <typename T, typename U>
void Interpreter<T,U>::PrintTokens(void)
{
    typename std::list<Token<T> >::iterator i = m_toklist.begin();
    while (i != m_toklist.end())
    {
        std::cout << i->Print();
        ++i;
    }
    std::cout << std::endl;
}

#undef PExpression

#endif
