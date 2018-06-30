#ifndef PERSISTENT_SET_H
#define PERSISTENT_SET_H

#include <iterator>
#include <cassert>
#include <memory>

//operator->
//iterator()

template<class T>
struct persistent_set {
private:
    struct node {
        std::shared_ptr<node> left, right;
        std::weak_ptr<node> parent;
        T *data;

        explicit node(T *data) noexcept : data(data) {}
    };

    friend std::shared_ptr<node> find_min(std::shared_ptr<node> head) noexcept {
        if (head == std::shared_ptr<node>()) {
            return head;
        }
        while (head->left != std::shared_ptr<node>()) {
            head = head->left;
        }
        return head;
    }

    friend std::shared_ptr<node> find_max(std::shared_ptr<node> head) noexcept {
        if (head == std::shared_ptr<node>()) {
            return head;
        }
        while (head->right != std::shared_ptr<node>()) {
            head = head->right;
        }
        return head;
    }

    friend std::shared_ptr<node> next_node(std::shared_ptr<node> n) noexcept {
        if (n == std::shared_ptr<node>()) {
            return n;
        }
        if (n->right == std::shared_ptr<node>()) {
            //find parent in witch left tree I
            while (n->parent.lock() != std::shared_ptr<node>()) {
                if (n->parent.lock()->left == n) {
                    break;
                }
                n = n->parent.lock();
            }
            return std::shared_ptr<node>(n->parent.lock());
        }
        return find_min(n->right);
    }

    friend std::shared_ptr<node> prev_node(std::shared_ptr<node> n) noexcept {
        if (n == std::shared_ptr<node>()) {
            return n;
        }
        if (n->left == std::shared_ptr<node>()) {
            //find parent in witch left tree I
            while (n->parent.lock() != std::shared_ptr<node>()) {
                if (n->parent.lock()->right == n) {
                    break;
                }
                n = n->parent.lock();
            }
            return std::shared_ptr<node>(n->parent.lock());
        }
        return find_max(n->left);
    }

public:
    struct iterator : std::iterator<std::bidirectional_iterator_tag, T const> {
        iterator() : head(), data() {}

        T const &operator*() const {
            return *data.lock()->data;
        }

        T const *operator->() const {
            return data.lock()->data;
        }

        iterator &operator++() {
            if (data.lock() == std::shared_ptr<node>()) {
                data = find_min(head.lock());
            } else {
                data = next_node(data.lock());
            }
            return *this;
        }

        iterator const operator++(int) {
            iterator r(data.lock(), head.lock());
            ++*this;
            return r;
        }

        iterator &operator--() {
            if (data.lock() == std::shared_ptr<node>()) {
                data = find_max(head.lock());
            } else {
                data = prev_node(data.lock());
            }
            return *this;
        }

        iterator const operator--(int) {
            iterator r(data.lock(), head.lock());
            --*this;
            return r;
        }

    private:
        std::weak_ptr<node> data;
        std::weak_ptr<node> head;

        explicit iterator(std::shared_ptr<node> data, std::shared_ptr<node> head) : data(data), head(head) {}

        friend struct persistent_set<T>;
    public:
        friend bool operator==(persistent_set<T>::iterator a, persistent_set<T>::iterator b) {
            return a.data.lock() == b.data.lock();
        }

        friend bool operator!=(persistent_set<T>::iterator a, persistent_set<T>::iterator b) {
            return !(a == b);
        }
    };

    typedef std::reverse_iterator<iterator> reverse_iterator;

    typedef iterator const_iterator;
    typedef reverse_iterator const_reverse_iterator;

    persistent_set() = default;

    persistent_set(persistent_set const &) = default;

    persistent_set &operator=(persistent_set const &rhs) = default;

    ~persistent_set() = default;

    iterator find(T const &data) noexcept {
        std::shared_ptr<node> tmp = head;
        while (tmp != std::shared_ptr<node>() && *tmp->data != data) {
            if (data > *tmp->data) {
                tmp = tmp->left;
            } else {
                tmp = tmp->right;
            }
        }
        return iterator(tmp, head);
    }

    std::pair<iterator, bool> insert(T const &data) {
        if (empty()) {
            head = std::shared_ptr<node>(new node(new T(data)));
            return std::pair<iterator, bool>(iterator(head, head), true);
        }
        iterator it = find(data);
        if (it != end()) {
            return std::pair<iterator, bool>(it, false);
        }

        std::shared_ptr<node> new_head(new node(new T(*head->data)));
        std::shared_ptr<node> new_cur = new_head;
        std::shared_ptr<node> old_cur = head;

        while (true) {
            if (*new_cur->data < data) {
                if (old_cur->left == std::shared_ptr<node>()) break;
                new_cur->left = std::shared_ptr<node>(new node(new T(*old_cur->left->data)));
                new_cur->right = old_cur->right;
                new_cur->left->parent = new_cur;
                new_cur = new_cur->left;
                old_cur = old_cur->left;
            } else {
                if (old_cur->right == std::shared_ptr<node>()) break;
                new_cur->right = std::shared_ptr<node>(new node(new T(*old_cur->right->data)));
                new_cur->left = old_cur->left;
                new_cur->right->parent = new_cur;
                new_cur = new_cur->right;
                old_cur = old_cur->right;
            }
        }

        if (*new_cur->data < data) {
            new_cur->left = std::shared_ptr<node>(new node(new T(data)));
            new_cur->right = old_cur->right;
            new_cur->left->parent = new_cur;
            new_cur = new_cur->left;
        } else {
            new_cur->right = std::shared_ptr<node>(new node(new T(data)));
            new_cur->left = old_cur->left;
            new_cur->right->parent = new_cur;
            new_cur = new_cur->right;
        }

        head = new_head;

        return std::pair<iterator, bool>(iterator(new_cur, head), true);
    }

    void erase(iterator it) {
        if (empty() || find(*it) == end()) {
            return;
        }

        std::shared_ptr<node> new_head(new node(new T(*head->data)));
        std::shared_ptr<node> new_cur = new_head;
        std::shared_ptr<node> old_cur = head;

        while (*new_cur->data != *it) {
            if (*new_cur->data < *it) {
                if (old_cur->left == std::shared_ptr<node>()) break;
                new_cur->left = std::shared_ptr<node>(new node(new T(*old_cur->left->data)));
                new_cur->right = old_cur->right;
                new_cur->left->parent = new_cur;
                new_cur = new_cur->left;
                old_cur = old_cur->left;
            } else {
                if (old_cur->right == std::shared_ptr<node>()) break;
                new_cur->right = std::shared_ptr<node>(new node(new T(*old_cur->right->data)));
                new_cur->left = old_cur->left;
                new_cur->right->parent = new_cur;
                new_cur = new_cur->right;
                old_cur = old_cur->right;
            }
        }
        new_cur.reset();
        head = new_head;
    }

    bool empty() {
        return head == std::shared_ptr<node>();
    }

    iterator begin() const {
        return iterator(find_min(head), head);
    }

    iterator end() const {
        return iterator(std::shared_ptr<node>(), head);
    }

    const_iterator cbegin() const {
        return begin();
    }

    const_iterator cend() const {
        return end();
    }

    reverse_iterator rbegin() const {
        return reverse_iterator(end());
    }

    reverse_iterator rend() const {
        return reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return rbegin();
    }

    const_reverse_iterator crend() const {
        return rend();
    }

private:
    std::shared_ptr<node> head;
};

template<class T>
void swap(persistent_set<T> &a, persistent_set<T> &b) {
    std::swap(a, b);
}

#endif //PERSISTENT_SET_H