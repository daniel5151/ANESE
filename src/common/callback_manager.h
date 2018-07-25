#pragma once

//
// Generic callback manager
//
// Takes callback arguments as template parameters
//
// eg: if the callback should be `cb(void* userdata, int my_data)`
//     define a `CallbackManager<int> int_callbacks;`
//

template <typename ...cb_args>
class CallbackManager {
public:
  using cb_t = void(*)(void* userdata, cb_args...);

private:
  struct cb_node {
    cb_t  cb = nullptr;
    void* userdata = nullptr;
    cb_node* next = nullptr;

    cb_node() = delete;
    cb_node(cb_t cb, void* userdata)
    : cb {cb}
    , userdata {userdata}
    {}
  };

  // linked list
  cb_node* cbs = nullptr;

public:
  ~CallbackManager() {
    while (this->cbs) {
      cb_node* n = this->cbs;
      this->cbs = cbs->next;
      delete n;
    }
  }

  void add_cb(cb_t function, void* userdata) {
    cb_node* n = new cb_node (function, userdata);
    if (!this->cbs) this->cbs = n;
    else {
      n->next = this->cbs;
      this->cbs = n;
    }
  }

  void run(cb_args... args) const {
    cb_node* n = this->cbs;
    while (n) {
      n->cb(n->userdata, args...);
      n = n->next;
    }
  }
};
