/*
  liblightgrep: not the worst forensics regexp engine
  Copyright (C) 2013, Lightbox Technologies, Inc

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "basic.h"

#include <algorithm>
#include <limits>

template <typename T>
class VectorFamily {
private:
  typedef typename std::vector<T> Vec;

  enum WhichType {
    ZERO = 0,
    ONE  = 1,
    MANY = ONE << 1
  };

public:
#pragma pack(push, 1)
  struct Holder {
    typedef typename std::vector<Vec>::size_type size_type;
    typedef T* iterator;
    typedef T const* const_iterator;

    T What;
    byte Which;

    Holder(): What(std::numeric_limits<T>::max()), Which(ZERO) {}
  };
#pragma pack(pop)

  typedef Holder ListType;

  typename Holder::size_type size(const Holder& l) const {
    switch (l.Which) {
    case ZERO:
      return 0;
    case ONE:
      return 1;
    default:
      return Store[l.What].size();
    }
  }

  void add(Holder& l, T e) {
    switch (l.Which) {
    case ZERO:
      l.Which = ONE;
      l.What = e;
      break;
    case ONE:
      {
        l.Which = MANY;
        const T tmp[2] = { l.What, e };
        Store.emplace_back(&tmp[0], &tmp[2]);
        l.What = Store.size() - 1;
      }
      break;
    default:
      Store[l.What].push_back(e);
    }
  }

  void insert(Holder& l, typename Holder::size_type i, T e) {
    switch (l.Which) {
    case ZERO:
      l.Which = ONE;
      l.What = e;
      break;
    case ONE:
      {
        l.Which = MANY;

        T tmp[2];
        if (i == 0) {
          tmp[0] = e;
          tmp[1] = l.What;
        }
        else {
          tmp[0] = l.What;
          tmp[1] = e;
        }

        Store.emplace_back(&tmp[0], &tmp[2]);
        l.What = Store.size() - 1;
      }
      break;
    default:
      {
        Vec& elist(Store[l.What]);
        elist.insert(i < elist.size() ? elist.begin() + i : elist.end(), e);
      }
    }
  }

  void remove(Holder& l, T e) {
    switch (l.Which) {
    case ZERO:
      {
        THROW_RUNTIME_ERROR_WITH_OUTPUT(e << " not in list");
      }
    case ONE:
      if (l.What != e) {
        THROW_RUNTIME_ERROR_WITH_OUTPUT(e << " not in list");
      }

      l.Which = ZERO;
      break;
    default:
      {
        Vec& edlist(Store[l.What]);

        const typename Vec::iterator i(std::find(edlist.begin(), edlist.end(), e));
        if (i == edlist.end()) {
          THROW_RUNTIME_ERROR_WITH_OUTPUT(e << " not in list");
        }

        edlist.erase(i);

        if (edlist.size() == 1) {
          l.Which = ONE;
          l.What = edlist.front();
          edlist.clear();
        }
      }
    }
  }

  void clear(Holder& l) {
    switch (l.Which) {
    case MANY:
      Store[l.What].clear();
    case ONE:
      l.Which = ZERO;
    }
  }

  T& at(Holder& l, typename Holder::size_type i) {
    switch (l.Which) {
    case ZERO:
      {
        THROW_RUNTIME_ERROR_WITH_OUTPUT(i << " out of bounds");
      }
    case ONE:
      return l.What;
    default:
      return Store[l.What][i];
    }
  }

  const T& at(const Holder& l, typename Holder::size_type i) const {
    switch (l.Which) {
    case ZERO:
      {
        THROW_RUNTIME_ERROR_WITH_OUTPUT(i << " out of bounds");
      }
    case ONE:
      return l.What;
    default:
      return Store[l.What][i];
    }
  }

  typename Holder::iterator find(Holder& l, T e) {
    switch (l.Which) {
    case ZERO:
      return end(l);
    case ONE:
      return l.What == e ? begin(l) : end(l);
    default:
      return &*std::find(Store[l.What].begin(), Store[l.What].end(), e);
    }
  }

  typename Holder::const_iterator find(const Holder& l, T e) const {
    switch (l.Which) {
    case ZERO:
      return end(l);
    case ONE:
      return l.What == e ? begin(l) : end(l);
    default:
      return &*std::find(Store[l.What].begin(), Store[l.What].end(), e);
    }
  }

  typename Holder::iterator begin(Holder& l) {
    switch (l.Which) {
    case ZERO:
    case ONE:
      return &l.What;
    default:
      return &*Store[l.What].begin();
    }
  }

  typename Holder::const_iterator begin(const Holder& l) const {
    switch (l.Which) {
    case ZERO:
    case ONE:
      return &l.What;
    default:
      return &*Store[l.What].begin();
    }
  }

  typename Holder::iterator end(Holder& l) {
    switch (l.Which) {
    case ZERO:
      return &l.What;
    case ONE:
      return &l.What + 1;
    default:
      return &*Store[l.What].end();
    }
  }

  typename Holder::const_iterator end(const Holder& l) const {
    switch (l.Which) {
    case ZERO:
      return &l.What;
    case ONE:
      return &l.What + 1;
    default:
      return &*Store[l.What].end();
    }
  }

private:
  std::vector<Vec> Store;
};
