#pragma once

#include <util/generic/vector.h>
#include <util/generic/set.h>


//required for bison - to make filling of vectors more convenient
template<class TYPE>
class simple_list
{
public:
    TYPE info;
    simple_list<TYPE>* next;

    simple_list(): info(), next(NULL) {};

/*    ~simple_list()
    {
        if (next != NULL)
        {
            delete next;
            next = NULL;
        }
    }
*/
    simple_list<TYPE>* add_new(TYPE _info)
    {
        simple_list<TYPE>* new_el = new simple_list;
        new_el->info = _info;
        new_el->next = this;
        return new_el;
    }

};

template<class TYPE>
void free_item(simple_list<TYPE>* item)
{
    delete item;
}

template<class TYPE>
void free_item(simple_list< TYPE* >* item)      // TYPE* specialization
{
    delete item->info;
    delete item;
}

template<class TYPE>
void free_list(simple_list<TYPE>* head)
{
    while (head != NULL) {
        simple_list<TYPE>* tmp = head;
        head = head->next;
        free_item(tmp);
    }
}

/*
//transfers data from simple_list<TYPE> into vector<TYPE>
template<class TYPE>
yvector<TYPE> convert_to_vector(simple_list<TYPE>* head, bool b_delete = true)
{
    yvector<TYPE> res;
    simple_list<TYPE>* tmp = head;
    for(; tmp != NULL ; tmp = tmp->next)
        res.push_back(tmp->info);

    if (b_delete)
        free_list(head);

    return res;
}
*/

template<class TYPE>
void fill_vector_and_free(yvector<TYPE>& res, simple_list<TYPE>* head)
{
    res.clear();
    for (simple_list<TYPE>* tmp = head; tmp != NULL; tmp = tmp->next)
        res.push_back(tmp->info);
    free_list(head);
}

template<class TYPE>
void fill_vector_and_free(yvector<TYPE>& res, simple_list<TYPE*>* head) // TYPE* specialization for head
{
    res.clear();
    for (simple_list<TYPE*>* tmp = head; tmp != NULL; tmp = tmp->next)
        res.push_back(*(tmp->info));
    free_list(head);
}

inline void fill_str_vector_and_free(yvector<Wtroka>& res, simple_list<Stroka>* head)
{
    res.clear();
    for (const simple_list<Stroka>* tmp = head; tmp != NULL; tmp = tmp->next)
        res.push_back(NStr::DecodeAuxDic(tmp->info));
    free_list(head);
}

inline void fill_str_vector_and_free(yvector< yvector<Wtroka> >& res, simple_list< yvector<Stroka> >* head)
{
    res.clear();
    for (const simple_list< yvector<Stroka> >* tmp = head; tmp != NULL; tmp = tmp->next) {
        res.push_back();
        yvector<Wtroka>& vec = res.back();
        for (size_t i = 0; i < tmp->info.size(); ++i)
            vec.push_back(NStr::DecodeAuxDic(tmp->info[i]));
    }
    free_list(head);
}

template<class TYPE>
void fill_set_and_free(yset<TYPE>& res, simple_list<TYPE>* head)
{
    res.clear();
    for (simple_list<TYPE>* tmp = head; tmp != NULL; tmp = tmp->next)
        res.insert(tmp->info);
    free_list(head);
}

template<class TYPE>
void fill_set_and_free(yset<TYPE>& res, simple_list< TYPE* >* head) // TYPE* specialization for head
{
    res.clear();
    for (simple_list<TYPE*>* tmp = head; tmp != NULL; tmp = tmp->next)
        res.insert(*(tmp->info));
    free_list(head);
}
