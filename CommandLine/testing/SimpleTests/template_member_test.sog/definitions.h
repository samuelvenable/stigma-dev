// A class template on a definitions page. The parser must build a real
// instantiation for map<K, V> so the semantic annotator can classify member
// accesses on variables of the instantiated type.
template<typename K, typename V>
struct map {
  int count;
  V slot;
  int size() { return count; }
  V& at(K) { return slot; }
  V& operator[](K) { ++count; return slot; }
};
