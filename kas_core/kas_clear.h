#ifndef KAS_CORE_KAS_CLEAR_H
#define KAS_CORE_KAS_CLEAR_H


#include <vector>


namespace kas::core
{
   struct kas_clear
   {
        using CLEAR_FN = void(*)();

    private:
        static auto& vector()
        {
            static auto vector_ = new std::vector<CLEAR_FN>();
            return *vector_;
        }

        static void add(CLEAR_FN fn)
        {
            vector().push_back(fn);
        }

    public:
        kas_clear(CLEAR_FN fn) 
        {
            //std::cout << __FUNCTION__ << ": add: " << (void *)fn << std::endl;
            add(fn);
        }

        // execute all the "clear" functions
        static void clear()
        {
            for (auto fn : vector())
                fn();
        }

   };

}

#endif
