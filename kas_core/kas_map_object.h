#ifndef KAS_CORE_CORE_MAP_OBJECT_H
#define KAS_CORE_CORE_MAP_OBJECT_H

#include "kas_object.h"

namespace kas::core
{

template <typename Derived
        , typename key_t
        , typename index_t = unsigned
        , template <typename, typename> class OBSTACK = std::deque
        , typename Allocator = std::allocator<Derived>>
struct kas_map_object
{
        //using base_t    = kas_map_object;	
        using derived_t = Derived;

        // for `expr_fits`
        using emits_value = std::true_type;     // object holds value

        // prevent copying or moving base_t instance
        kas_map_object(kas_map_object const&)            = delete;
        kas_map_object(kas_map_object&&)                 = delete;
        kas_map_object& operator=(kas_map_object const&) = delete;
        kas_map_object& operator=(kas_map_object&&)      = delete;

    private:
        static auto& map()
        {
            // map: key -> obstack_index
            static auto map_ = new std::map<key_t, index_t>;
            return *map_;
        }

        static auto& obstack()
        {
            static auto obstack_ = new OBSTACK<Derived, Allocator>;
            return *obstack_;
        }

    public:
        kas_map_object() = default;

        static derived_t& lookup(key_t key)
        {
            auto iter = map().find(key);
            if (iter == map().end())
                throw std::runtime_error{"kas_map_opject::lookup: key not found"};
            return obstack()[iter->second - 1];
        }

        static derived_t& get(index_t index)
        {
            return obstack()[index - 1];
        }

    
        using result_t = std::pair<Derived&, bool>;

        template <typename...Ts>
        static result_t add(key_t key, Ts&&...args)
        {
            index_t insert_index = obstack().size() + 1;
            auto result = map().emplace(key, insert_index);

            // if already mapped (not inserted), return result
            if (result.second == false)
                return { get(result.first->second), false };

            auto& obj = obstack().emplace_back(std::forward<Ts>(args)...);
            obj.index = insert_index;
            obj.key_p = &result.first->first;
            return { obj, true };
        }

        static auto size()
        {
            return obstack().size();
        }

        auto index() const { return  k_index; }
        auto key()   const { return *k_key_p; }
    
    protected:
        // method to walk thru and modify the `derived_t` table in *key* order
        // NB: pass a mutable reference to `derived_t` fo `fn`
        template <typename FN>
        static void key_for_each(FN fn)
        {
            for (auto &obj : map())
                fn(get(obj.second));
        }
     
        // method to walk thru and modify the `derived_t` table in alloc order
        // iterate from index `start` to end
        // NB: a mutable reference to `derived_t` fo `fn`
        template <typename FN>
        static void idx_for_each(FN fn, index_t start = {})
        {
            auto it  = obstack().begin();
            auto end = obstack().end();

            std::advance(it, start);
            while (it != end) {
                auto& obj = *it++;
                fn(obj);
            }
        }

    private: 
        // test fixture support: deallocate all instances
        static void clear() {}      // default `derived_t::clear()`
        static void obj_clear()
        {
            map().clear();
            obstack().clear();
            derived_t::clear();
        }

        static inline kas_clear _c{obj_clear};

    private:
        index_t k_index;
        key_t  *k_key_p;

};
}

#endif
