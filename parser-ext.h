#ifndef PARSEREXT
#define PARSEREXT

//== ВКЛЮЧЕНИЯ.
#include <../Z-Server/TinyXML2/tinyxml2.h>
#include <list>
#ifndef WIN32
#include <algorithm>
#endif

//== ПРОСТРАНСТВА.
using namespace tinyxml2;
using namespace std;

//== МАКРОСЫ.
#define FCN_ONE_LEVEL   true
#define FCN_MULT_LEVELS false
#define FCN_FIRST_ONLY  true
#define FCN_ALL         false
#define PARSE_CHILDLIST(Node, List, Name, Level, FName)         \
    list <XMLNode*>* List = new list<XMLNode*>();               \
    FindChildNodes(Node, *List, Name, Level, FCN_ALL);          \
    while(!List->empty())                                       \
    {                                                           \
        XMLNode* FName;                                         \
        FName = List->front();
#define PARSE_CHILDLIST_END(List)                               \
    List->pop_front();                                          \
    }                                                           \
    delete List

#define FIND_IN_CHILDLIST(Node, List, Name, Level, FName)       \
    list <XMLNode*>* List = new list<XMLNode*>();               \
    if(FindChildNodes(Node, *List, Name, Level, FCN_FIRST_ONLY))\
    {                                                           \
        XMLNode* FName;                                         \
        FName = List->front();
#define FIND_IN_CHILDLIST_END(List) PARSE_CHILDLIST_END(List)

//== ФУНКЦИИ.
/// Поиск наследующих разъёмов.
bool FindChildNodes(XMLNode* p_NodeExt, list <XMLNode*> &a_lFoundedElements, const char* p_chTarget,
						   bool bOneLevel = false, bool bFirstOnly = false);
												///< \param[in] p_NodeExt Указатель на разъём родителя.
												///< \param[out] a_lFoundedElements Ссылка на список для заполнения.
												///< \param[in] p_chTarget Ссылка имя искомого разъёма.
												///< \param[in] bOneLevel Флаг прохода по одному уровню.
												///< \param[in] bFirstOnly Флаг единовременного срабатывания.
												///< \return true, если удачно.


#endif // PARSEREXT
