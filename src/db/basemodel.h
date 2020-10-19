#ifndef BASEMODEL_H
#define BASEMODEL_H

#include <QAbstractTableModel>

template <typename T>
struct BaseModel : public QAbstractTableModel {
  using ID = typename T::ID;
  using set_t = typename T::Database;

  int rowCount(const QModelIndex& = QModelIndex()) const override {
    return _data.size();
  }

  const T& at (ID id) const {
    return *_data.find(id);
  }

  T& at (ID id) {
    return const_cast<T&>(
      const_cast<const BaseModel*>(this)->at(id));
  }

  const T& atIndex (int i) const {
    auto it = _data.begin();
    std::advance(it, i);
    return *it;
  }

  T& atIndex (int i) {
    return const_cast<T&>(
      const_cast<const BaseModel*>(this)->atIndex(i));
  }

  int indexOf (ID id) const {
    auto it = _data.find(id);
    if (it == _data.end())
      throw std::invalid_argument("No value for given id");
    return std::distance(_data.begin(), it);
  }

  ID nextIDNoIncrement (void) const {
    return _nextID;
  }

  virtual void valueModified (ID id) = 0;

protected:
  set_t _data;

  ID _nextID = ID(1);
  ID nextID (void) {
    auto v = _nextID;
    _nextID = ID(int(_nextID)+1);
    return v;
  }
};

#endif // BASEMODEL_H
