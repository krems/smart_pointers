// Author: Krems

#include <iostream>

using std::cout;
using std::cin;
using std::endl;

template<typename T>
class ArrayStorageStrategy {
 public:
  ArrayStorageStrategy(T* pointer): ptr(pointer) {}
  
  virtual ~ArrayStorageStrategy() {
    deletePtr();
  }

  T operator [] (size_t pos) const {
    return *(ptr + pos);
  }

  T& operator [] (size_t pos) {
    return *(ptr + pos);
  }

 protected:
  void deletePtr() {
    delete[] ptr;
  }
  
 protected:
  T* ptr;
};

template<typename T>
class SinglePtrStorageStrategy {
 public:
  SinglePtrStorageStrategy(T* pointer): ptr(pointer) {}
  
  virtual ~SinglePtrStorageStrategy() {
    deletePtr();
  }

 protected:
  void deletePtr() {
    delete ptr;
  }
  
 protected:
  T* ptr;
};

// Ld fail on copy constructor or operator = usage try
// Attention! Do not use with stl containers!
template<typename T, template<typename> class StorageStrategy>
class RestrictCopyOwnershipStrategy: public StorageStrategy<T> {
 public:
  RestrictCopyOwnershipStrategy(T* pointer): StorageStrategy<T>(pointer) {}
 protected:
  RestrictCopyOwnershipStrategy& operator =(const
                                            RestrictCopyOwnershipStrategy&);
  RestrictCopyOwnershipStrategy(const RestrictCopyOwnershipStrategy&);
};

// Helps to organize ref counting
struct ReferenceCounter {
 private:
  size_t count;
 public:
  void addReference() {
    ++count;
  }

  size_t release() {
    return --count;
  }
};

// Counting in external shared structure
template<typename T, template<typename> class StorageStrategy>
class RefCounterOwnershipStrategy: public StorageStrategy<T> {
 public:
  RefCounterOwnershipStrategy(T* pointer): StorageStrategy<T>(pointer) {
    counter = new ReferenceCounter;
    counter->addReference();
  }
  
  RefCounterOwnershipStrategy& operator =(RefCounterOwnershipStrategy other) {
    swap(*this, other);
    return *this;    
  }
  
  RefCounterOwnershipStrategy(const RefCounterOwnershipStrategy& other):
      StorageStrategy<T>(other.ptr) {
    counter = other.counter;
    counter->addReference();
  }

  ~RefCounterOwnershipStrategy() {
    if (counter->release() != 0) {
      ptr = NULL;
      counter = NULL;
    } else {
      delete counter;
    }
  }
 protected:
  StorageStrategy<T>::ptr;
  ReferenceCounter* counter;
 private:
  void swap(RefCounterOwnershipStrategy& lhs,
            RefCounterOwnershipStrategy& rhs) {
    std::swap(lhs.counter, rhs.counter);
    std::swap(lhs.ptr, rhs.ptr);
  }
};

// Helps to organize LinkedList
struct Node {
  Node* next;
  Node* prev;
};

// Circular references
template<typename T, template<typename> class StorageStrategy>
class LinkedRefOwnershipStrategy: public StorageStrategy<T> {
 public:
  LinkedRefOwnershipStrategy(T* pointer): StorageStrategy<T>(pointer) {
    node = new Node;
    node->next = node;
    node->prev = node;
  }
  
  LinkedRefOwnershipStrategy& operator =(LinkedRefOwnershipStrategy other) {
    swap(*this, other);
    return *this;
  }
  
  LinkedRefOwnershipStrategy(const LinkedRefOwnershipStrategy& other):
      StorageStrategy<T>(other.ptr) {
    node = new Node;
    node->next = other.node;
    node->prev = (other.node)->prev;
    (other.node)->prev = node;
    (node->prev)->next = node;
  }

  ~LinkedRefOwnershipStrategy() {
    if (node->next != node) {
      (node->prev)->next = node->next;
      (node->next)->prev = node->prev;
      ptr = NULL;
    }
    delete node;
  }
 protected:
  StorageStrategy<T>::ptr;
  Node* node;
 private:
  void swap(LinkedRefOwnershipStrategy& lhs, LinkedRefOwnershipStrategy& rhs) {
    std::swap(lhs.node, rhs.node);
    std::swap(lhs.ptr, rhs.ptr);
  }
};

// auto_ptr simplier version
//Attention! Do not use with stl containers! Do not pass by value!
template<typename T, template<typename> class StorageStrategy>
class DelegateOwnershipStrategy: public StorageStrategy<T> {
 protected:
  void reassign(T* newPtr) {
    if (newPtr != ptr) {
      StorageStrategy<T>::deletePtr();
      ptr = newPtr;
    }
  }
  
  T* free() {
    T* tmp = ptr;
    ptr = NULL;
    return tmp;
  }
 public:
  DelegateOwnershipStrategy(T* pointer): StorageStrategy<T>(pointer) {}
  
  DelegateOwnershipStrategy& operator =(DelegateOwnershipStrategy& other) {
    reassign(other.free());
    return *this;
  }
  
  DelegateOwnershipStrategy(DelegateOwnershipStrategy& other):
      StorageStrategy<T>(other.free()) {}
 protected:
  StorageStrategy<T>::ptr;
};

template<typename T,
         template<typename> class StorageStrategy = SinglePtrStorageStrategy,
         template<typename, template<typename> class > class
         OwnershipStrategy = RestrictCopyOwnershipStrategy>
class SmartPointer: public OwnershipStrategy<T, StorageStrategy> {
 public:
  explicit SmartPointer(T* pointer):
      OwnershipStrategy<T, StorageStrategy>(pointer) {}

  T& operator *() const {
    return *ptr;
  }

  T* operator ->() const {
    return ptr;
  }

  typedef T element_type;
 private:
  StorageStrategy<T>::ptr;
};

int main() {
  SmartPointer<int> sm_ptr(new int);
  *sm_ptr = 3;
  cout << *sm_ptr << endl;
  
  SmartPointer<int, SinglePtrStorageStrategy, DelegateOwnershipStrategy>
      sm_ptr_1(new int);
  *sm_ptr_1 = 5;
  cout << *sm_ptr_1 << endl;
  {
    SmartPointer<int, SinglePtrStorageStrategy, DelegateOwnershipStrategy>
        sm_ptr_1_1(sm_ptr_1);
    cout << *sm_ptr_1_1 << endl;
    *sm_ptr_1_1 = -12;
  }
  // cout << *sm_ptr_1 << endl; // seg fault as I wished
  SmartPointer<int, SinglePtrStorageStrategy, DelegateOwnershipStrategy>
      sm_ptr_1_2(new int);
  *sm_ptr_1_2 = 1;
  sm_ptr_1 = sm_ptr_1_2;
  cout << *sm_ptr_1 << endl;
  
  SmartPointer<int, SinglePtrStorageStrategy, RefCounterOwnershipStrategy>
      sm_ptr_2(new int);
  *sm_ptr_2 = -9;
  cout << *sm_ptr_2 << endl;
  {
    SmartPointer<int, SinglePtrStorageStrategy, RefCounterOwnershipStrategy>
        sm_ptr_2_1(sm_ptr_2);
    cout << *sm_ptr_2_1 << endl;
    *sm_ptr_2_1 = 12;
  }
  cout << *sm_ptr_2 << endl;
  SmartPointer<int, SinglePtrStorageStrategy, RefCounterOwnershipStrategy>
      sm_ptr_2_2(new int);
  *sm_ptr_2_2 = 13;
  sm_ptr_2 = sm_ptr_2_2;
  cout << *sm_ptr_2 << endl;

  SmartPointer<int, SinglePtrStorageStrategy, LinkedRefOwnershipStrategy>
      sm_ptr_3(new int);
  *sm_ptr_3 = -99;
  cout << *sm_ptr_3 << endl;
  {
    SmartPointer<int, SinglePtrStorageStrategy, LinkedRefOwnershipStrategy>
        sm_ptr_3_1(sm_ptr_3);
    cout << *sm_ptr_3_1 << endl;
    *sm_ptr_3_1 = 44;
  }
  cout << *sm_ptr_3 << endl;
  SmartPointer<int, SinglePtrStorageStrategy, LinkedRefOwnershipStrategy>
      sm_ptr_3_2(new int);
  *sm_ptr_3_2 = 66;
  sm_ptr_3 = sm_ptr_3_2;
  cout << *sm_ptr_3 << endl;
  //------------------------Array--------------------------------------------
  SmartPointer<int, ArrayStorageStrategy, LinkedRefOwnershipStrategy>
      sm_ptra(new int[3]);
  *sm_ptra = 3;
  sm_ptra[1] = 2;
  sm_ptra[2] = 1;
  cout << *sm_ptra << " " << sm_ptra[1] << " " << sm_ptra[2] << endl;
  
  return 0;
}
