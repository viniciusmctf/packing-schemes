/* Copyright Vinicius F. @ UFSC [2016]

 Contact: vinicius.mctf@grad.ufsc.br

 */

#ifndef _ORDERED_ELEMENT_H_
#define _ORDERED_ELEMENT_H_

/* This is a load balancing entity, used for
    load balancing heaps and ordered queues.
 */
struct Element {
    Element(int id, double load, int hash) : id(id), load(load), hash(hash) {}
    Element(int id, double load) : id(id), load(load), hash(0) {}
  //  Element(Element& e) : id(e.id), load(e.load) {}
    Element() {}
    int id;
    int hash;
    double load;
};

inline bool operator<(const Element& lhs, const Element& rhs) {
    return lhs.load < rhs.load;
}

inline bool operator>(const Element& lhs, const Element& rhs) {
    return lhs.load > rhs.load;
}

#endif /* _ORDERED_ELEMENT_H_ */
