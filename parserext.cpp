//== ВКЛЮЧЕНИЯ.
#include "parserext.h"

//== ФУНКЦИИ.
// Поиск наследующих разъёмов.
bool FindChildNodes(XMLNode* p_NodeExt, list <XMLNode*> &a_lFoundedElements, const char* p_chTarget,
						   bool bOneLevel, bool bFirstOnly)
{
	XMLNode* p_Node;
	unsigned int uiCount = 0;
	bool bFound = false;
	//
	p_Node = p_NodeExt->FirstChild();
	while(p_Node) // Пока есть.
	{
		if (p_Node->Value() && !strcmp((const char*)p_Node->Value(), p_chTarget)) // Есть знач., и оно равно задан.
		{
			bFound = true;
			if(uiCount == 0) a_lFoundedElements.clear();
			a_lFoundedElements.push_back(p_Node);
			uiCount++; // Счётчик поподаний в плюс.
			if (bFirstOnly) return bFound; // Если флаг - выход при первом попадании.
		}
		if(!bOneLevel)
		{
			if (p_Node->FirstChildElement()) p_Node = p_Node->FirstChildElement(); // Если есть дети - внутрь.
			else if (p_Node->NextSiblingElement())
				p_Node = p_Node->NextSiblingElement(); // Если есть следующий - дальше.
			else // И наче:
			{
				while(p_Node->Parent() && !p_Node->Parent()->NextSiblingElement())
					// Если есть род. и у род. нет соседа -
					p_Node = p_Node->Parent(); // Переход к родителю.
				if(p_Node->Parent() && p_Node->Parent()->NextSiblingElement()) // Если у род. есть сосед -
					p_Node = p_Node->Parent()->NextSiblingElement(); // Переход к соседу родителя.
				else
					break; // Иначе - всё.
			}
		}
		else
		{
			if (p_Node->NextSiblingElement()) p_Node = p_Node->NextSiblingElement(); // Если есть следующий - дальше.
			else return bFound;
		}
	}
	return bFound;
}
