#include <iostream>
#include <vector>
#include <utility>
#include <ctime>
#include <cstring>
#include <sys/types.h>
#include <regex.h>
#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <boost/preprocessor/cat.hpp>

///TODO callback when capture (open, close and fail)
///TODO transition multi-characters
///TODO regex compiler ("..." -> C++)
///TODO

namespace rndfa {
    const unsigned ANY_CHARACTER = 1 << 8;
    const unsigned SPLIT = 1 << 9;
    const unsigned CAPTURE_OPEN = 1 << 10;
    const unsigned CAPTURE_CLOSE = 1 << 11;
    const unsigned SPECIAL_CHECK = 1 << 12;

    struct StateBase
    {
        StateBase(unsigned c, StateBase * out1 = 0, StateBase * out2 = 0)
        : c(c)
        , id(0)
        , num(0)
        , out1(out1)
        , out2(out2)
        {}

        virtual ~StateBase(){}

        virtual bool check(int c)
        {
            /**///std::cout << num << ": " << char(this->c & ANY_CHARACTER ? '.' : this->c&0xFF);
            return (this->c & ANY_CHARACTER) || (this->c&0xFF) == c;
        }

        virtual void display(std::ostream& os)
        {
            if (this->c & ANY_CHARACTER) {
                os << "any";
            }
            else if (this->c & (SPLIT|CAPTURE_CLOSE|CAPTURE_OPEN)){
                os << (this->is_split() ? "(split)" : this->c == CAPTURE_OPEN ? "(open)" : "(close)");
            }
            else {
                os << "'" << char(this->c & 0xff) << "'";
            }
        }

        bool is_cap() const
        { return this->c & (CAPTURE_OPEN|CAPTURE_CLOSE); }

        bool is_cap_open() const
        { return this->c == CAPTURE_OPEN; }

        bool is_cap_close() const
        { return this->c == CAPTURE_CLOSE; }

        bool is_split() const
        { return this->c == SPLIT; }

        unsigned c;
        unsigned id;
        unsigned num;

        StateBase *out1;
        StateBase *out2;
    };

    std::ostream& operator<<(std::ostream& os, StateBase& st)
    {
        st.display(os);
        return os;
    }

    typedef StateBase State;

    struct StateRange : StateBase
    {
        StateRange(int r1, int r2, StateBase* out1 = 0, StateBase* out2 = 0)
        : StateBase(r1, out1, out2)
        , rend(r2)
        {}

        virtual ~StateRange()
        {}

        virtual bool check(int c)
        {
            /**///std::cout << char(this->c&0xFF) << "-" << char(rend);
            return (this->c&0xFF) <= c && c <= rend;
        }

        virtual void display(std::ostream& os)
        {
            os << "[" << char(this->c) << "-" << char(this->rend) << "]";
        }

        int rend;
    };

    State * range(char c1, char c2, StateBase* out = 0) {
        return new StateRange(c1, c2, out);
    }

    struct StateCharacters : StateBase
    {
        StateCharacters(const std::string& s, StateBase* out1 = 0, StateBase* out2 = 0)
        : StateBase(s[0], out1, out2)
        , str(s)
        {}

        virtual ~StateCharacters()
        {}

        virtual bool check(int c)
        {
            /**///std::cout << str;
            return this->str.find(c) != std::string::npos;
        }

        virtual void display(std::ostream& os)
        {
            os << "[" << this->str << "]";
        }

        std::string str;
    };

    struct StateMultiTest : StateBase
    {
        struct Checker {
            virtual bool check(int c) = 0;
            virtual void display(std::ostream& os) = 0;
        };

        std::vector<Checker*> checkers;
        typedef std::vector<Checker*>::iterator checker_iterator;


        StateMultiTest(StateBase* out1 = 0, StateBase* out2 = 0)
        : StateBase(SPECIAL_CHECK, out1, out2)
        , checkers()
        {}

        virtual ~StateMultiTest()
        {
            for (checker_iterator first = this->checkers.begin(), last = this->checkers.end(); first != last; ++first) {
                delete *first;
            }
        }

        virtual bool check(int c)
        {
            for (checker_iterator first = this->checkers.begin(), last = this->checkers.end(); first != last; ++first) {
                if ((*first)->check(c)) {
                    return true;
                }
            }
            return false;
        }

        virtual void display(std::ostream& os)
        {
            checker_iterator first = this->checkers.begin();
            checker_iterator last = this->checkers.end();
            if (first != last) {
                (*first)->display(os);
                while (++first != last) {
                    os << "|";
                    (*first)->display(os);
                }
            }
        }

        void push_checker(Checker * checker)
        {
            this->checkers.push_back(checker);
        }
    };

    struct CheckerString : StateMultiTest::Checker
    {
        CheckerString(const std::string& s)
        : str(s)
        {}

        virtual ~CheckerString()
        {}

        virtual bool check(int c)
        {
            return this->str.find(c) != std::string::npos;
        }

        virtual void display(std::ostream& os)
        {
            os << "[" << this->str << "]";
        }

        std::string str;
    };

    struct CheckerInterval : StateMultiTest::Checker
    {
        CheckerInterval(int first, int last)
        : begin(first)
        , end(last)
        {}

        virtual ~CheckerInterval()
        {}

        virtual bool check(int c)
        {
            return this->begin <= c && c <= this->end;
        }

        virtual void display(std::ostream& os)
        {
            os << "[" << char(this->begin) << "-" << char(this->end) << "]";
        }

        int begin;
        int end;
    };

    State * characters(const char * s, StateBase* out = 0) {
        return new StateCharacters(s, out);
    }

    //NOTE rename to Regex ?
    class StateMachine2
    {
        class RangeList;

        struct StateList
        {
            StateBase * st;
            RangeList * next;
        };

        StateList * st_list;

        struct RangeList
        {
            State * st;
            StateList * first;
            StateList * last;
            StateBase ** cap_open_first;
            StateBase ** cap_open_last;
            StateBase ** cap_close_first;
            StateBase ** cap_close_last;
        };

        RangeList * st_range_list;
        RangeList * st_range_list_last;

        struct IsCap
        {
            bool operator()(StateList& l) const
            {
                return l.st->is_cap();
            }
        };

    public:
        explicit StateMachine2(StateBase * st)
        : nb_capture(0)
        , idx_trace(-1u)
        , idx_trace_free(0)
        , pidx_trace_free(0)
        , st_first(st)
        , captures(0)
        , pcaptures(0)
        , captures_for_list(0)
        , pcaptures_for_list(0)
        , traces(0)
        , vec()
        , l1()
        , l2()
        {
            {
                this->push_state(st);
                this->nb_capture /= 2;
            }

            if (!this->vec.empty())
            {
                {
                    const unsigned matrix_size = this->vec.size() * this->vec.size();
                    this->st_list = new StateList[matrix_size];
                    std::memset(this->st_list, 0, matrix_size * sizeof * this->st_list);
                }

                {
                    const unsigned size = this->vec.size();
                    this->st_range_list = new RangeList[size];
                    this->st_range_list_last = this->st_range_list;
                    for (unsigned n = 0; n < size; ++n) {
                        RangeList& l = *this->st_range_list_last;
                        ++this->st_range_list_last;
                        l.st = 0;
                        l.first = this->st_list + n * this->vec.size();
                        l.last = l.first;
                    }
                }

                if (this->nb_capture) {
                    this->captures = new StateBase *[this->nb_capture*2];
                    this->pcaptures = this->captures;

                    for (state_iterator first = this->vec.begin(), last = this->vec.end(); first != last; ++first) {
                        if ((*first)->is_cap()) {
                            *this->pcaptures = *first;
                            ++this->pcaptures;
                        }
                    }

                    const unsigned col = this->vec.size() - this->nb_capture * 2;
                    const unsigned matrix_size = col * this->nb_capture * 2;
                    this->traces = new char const *[matrix_size];
                    this->idx_trace_free = new unsigned[col];
                }

                {
                    unsigned step = 0;
                    this->init_list(this->st_range_list, st, step);
                    while (this->st_range_list != this->st_range_list_last && 0 == (this->st_range_list_last-1)->st) {
                        --this->st_range_list_last;
                    }
                }

                if (this->nb_capture) {
                    unsigned nb_cap_list_size = 0;
                    //BEGIN init size
                    for (RangeList * l = this->st_range_list; l < this->st_range_list_last; ++l) {
                        for (StateList * first = l->first, * last = l->last; first < last; ++first) {
                            if (first->st->is_cap_open()) {
                                ++nb_cap_list_size;
                                ++l->cap_open_last;
                            }
                            else if (first->st->is_cap_close() && first->next) {
                                ++nb_cap_list_size;
                                ++first->next->cap_close_last;
                            }
                        }
                    }
                    //END init size
                    //BEGIN assign range
                    this->captures_for_list = new StateBase *[nb_cap_list_size];
                    this->pcaptures_for_list = this->captures_for_list;
                    for (RangeList * l = this->st_range_list; l < this->st_range_list_last; ++l) {
                        StateBase** const start = 0;
                        l->cap_open_first = this->pcaptures_for_list;
                        this->pcaptures_for_list += (l->cap_open_last - start);
                        l->cap_open_last = l->cap_open_first;
                        l->cap_close_first = this->pcaptures_for_list;
                        this->pcaptures_for_list += (l->cap_close_last - start);
                        l->cap_close_last = l->cap_close_first;
                    }
                    //END assign range
                    //BEGIN assign state
                    for (RangeList * l = this->st_range_list; l < st_range_list_last; ++l) {
                        for (StateList * first = l->first, * last = l->last; first < last; ++first) {
                            if (first->st->is_cap_open()) {
                                *l->cap_open_last = first->st;
                                ++l->cap_open_last;
                            }
                            else if (first->st->is_cap_close() && first->next) {
                                *l->cap_close_last = first->st;
                                ++l->cap_close_last;
                            }
                        }
                    }
//                     //END assign state
//                     //BEGIN clear capture
//                     for (RangeList * l = this->st_range_list; l->st; ++l) {
//                         l->last = std::remove_if(l->first, l->last, IsCap());
//                     }
//                     //END clear capture
                }

                l1.set_parray(new StateListByStep::Info[this->vec.size() * 2]);
                l2.set_parray(l1.array + this->vec.size());
            }
        }

        ~StateMachine2()
        {
            delete [] this->st_list;
            delete [] this->st_range_list;
            delete [] this->traces;
            delete [] this->captures;
            delete [] this->idx_trace_free;
            delete [] this->captures_for_list;
            delete [] this->l1.array;
        }

    private:
        unsigned pop_idx_trace(unsigned cp_idx)
        {
            --this->pidx_trace_free;
            assert(this->pidx_trace_free >= this->idx_trace_free);
            const unsigned size = (this->nb_capture * 2);
            char const ** from = this->traces + cp_idx * size;
            char const ** to = this->traces + *this->pidx_trace_free * size;
            for (char const ** last = to + size; to < last; ++to, ++from) {
                *to = *from;
            }
            return *this->pidx_trace_free;
        }

        void push_idx_trace(unsigned n)
        {
            assert(this->pidx_trace_free <= this->idx_trace_free + this->vec.size() - this->nb_capture * 2);
            *this->pidx_trace_free = n;
            ++this->pidx_trace_free;
        }

        void push_state(RangeList* l, StateBase * st, unsigned& step)
        {
            if (st && st->id != step) {
                st->id = step;
                if (st->is_split()) {
                    this->push_state(l, st->out1, step);
                    this->push_state(l, st->out2, step);
                }
                else {
                    l->last->st = st;
                    ++l->last;
                    if (st->is_cap()) {
                        this->push_state(l, st->out1, step);
                    }
                }
            }
        }

        RangeList* find_range_list(StateBase * st)
        {
            /**///std::cout << (__FUNCTION__) << std::endl;
            for (RangeList * l = this->st_range_list; l < this->st_range_list_last && l->st; ++l) {
                if (l->st == st) {
                    return l;
                }
            }
            return 0;
        }

        struct CompareStateList
        {
            bool operator()(const StateList&, const StateList& b) const {
                return b.st->c & ANY_CHARACTER;
            }
        };

        void init_list(RangeList* l, StateBase * st, unsigned& step)
        {
            /**///std::cout << (__FUNCTION__) << std::endl;
            l->st = st;
            l->cap_open_first = 0; ///NOTE optional
            l->cap_open_last = 0;
            l->cap_close_first = 0; ///NOTE optional
            l->cap_close_last = 0;
            this->push_state(l, st, step);
            /**///std::cout << "-- " << (l) << std::endl;
            for (StateList * first = l->first, * last = l->last; first < last; ++first) {
                /**///std::cout << first->st->num << ("\t") << first->st << ("\t") << first->next << std::endl;
                if (0 == first->st->out1) {
                    continue ;
                }
                RangeList * luse = this->find_range_list(first->st->out1);
                /**///std::cout << "[" << luse << "]" << std::endl;
                if (luse) {
                    first->next = luse;
                }
                else {
                    RangeList * le = l+1;
                    while (le < this->st_range_list_last && le->st) {
                        ++le;
                    }
                    first->next = le;
                    init_list(le, first->st->out1, ++step);
                }
            }

            std::sort<>(l->first, l->last, CompareStateList());
        }

    public:
        typedef std::pair<const char **, const char **> TraceRange;

        TraceRange get_trace() const
        {
            char const ** strace = this->traces + this->idx_trace * (this->nb_capture * 2);
            return TraceRange(strace, strace + (this->nb_capture * 2));
        }

        typedef std::pair<const char *, const char *> range_t;
        typedef std::vector<range_t> range_list;

        range_list exact_match(const char * s)
        {
            range_list ranges;

            if (this->nb_capture) {
                if (Matching(*this).exact_match(s)) {
                    this->append_match_result(ranges);
                }
            }

            return ranges;
        }

        bool exact_search(const char * s)
        {
            if (!this->st_first) {
                return false;
            }
            return Searching(*this).exact_match(s);
        }

        bool exact_search_with_trace(const char * s)
        {
            if (this->nb_capture == 0) {
                return exact_search(s);
            }
            return Matching(*this).exact_match(s);
        }

        range_list match_result()
        {
            range_list ret;
            this->append_match_result(ret);
            return ret;
        }

        void append_match_result(range_list& ranges) const
        {
            ranges.reserve(this->nb_capture);
            TraceRange trace = this->get_trace();

            StateBase ** pst = this->captures;
            while (pst < this->pcaptures) {
                while ((*pst)->is_cap_close()) {
                    if (++pst >= this->pcaptures) {
                        return ;
                    }
                }
                StateBase ** pbst = pst;
                unsigned n = 1;
                while (++pst < this->pcaptures && ((*pst)->is_cap_open() ? ++n : --n)) {
                }
                ranges.push_back(range_t(
                    trace.first[pbst - this->captures],
                    trace.first[pst - this->captures]
                ));
                pst = ++pbst;
            }
        }

    private:
        void push_state(StateBase * st)
        {
            if (st && st->id != -1u) {
                st->id = -1u;
                st->num = this->vec.size();
                if (st->is_cap()) {
                    st->num = this->nb_capture;
                    ++this->nb_capture;
                }
                this->vec.push_back(st);
                this->push_state(st->out1);
                if (st->is_split()) {
                    this->push_state(st->out2);
                }
            }
        }

    private:
        struct StateListByStep
        {
            struct Info {
                RangeList * rl;
                unsigned idx;
            };

            StateListByStep()
            : array(0)
            {}

            void push_back(RangeList* val, unsigned idx)
            {
                this->parray->rl = val;
                this->parray->idx = idx;
                ++this->parray;
            }

            Info& operator[](int n) const
            { return this->array[n]; }

            Info * begin() const
            { return this->array; }

            Info * end() const
            { return this->parray; }

            void set_parray(Info * p)
            {
                this->array = p;
                this->parray = p;
            }

            bool empty() const
            { return this->array == this->parray; }

            std::size_t size() const
            { return this->parray - this->array; }

            void clear()
            { this->parray = this->array; }

            Info * array;
            Info * parray;
        };

        struct Searching
        {
            StateMachine2 &sm;

            Searching(StateMachine2& sm)
            : sm(sm)
            {}

            typedef StateListByStep::Info Info;

            RangeList * step(const char *s, RangeList * l)
            {
                for (StateList * first = l->first, * last = l->last; first != last; ++first) {
                    if (!first->st->is_cap() && first->st->check(*s)) {
#ifdef DISPLAY_TRACE
                        this->sm.display_elem_state_list(*first, 0);
#endif
                        return first->next;
                    }
                }
                return (RangeList*)1;
            }

            bool exact_match(const char * s)
            {
#ifdef DISPLAY_TRACE
                this->sm.display_dfa();
#endif

                RangeList * l = this->sm.st_range_list;

                for(; *s && l > (void*)1; ++s){
#ifdef DISPLAY_TRACE
                    std::cout << "\033[01;31mc: '" << *s << "'\033[0m\n";
#endif
                    l = this->step(s, l);
                }

                return 0 == l;
            }
        };

    private:
        void reset_trace()
        {
            for (RangeList * l = this->st_range_list; l < this->st_range_list_last; ++l) {
                l->st->id = 0;
            }
            this->pidx_trace_free = this->idx_trace_free;
            const unsigned size = this->vec.size() - this->nb_capture * 2;
            for (unsigned i = 0; i < size; ++i, ++this->pidx_trace_free) {
                *this->pidx_trace_free = i;
            }
            std::memset(this->traces, 0,
                        size * this->nb_capture * 2 * sizeof this->traces[0]);
        }

    public:
        void display_elem_state_list(const StateList& e, unsigned idx) const
        {
            std::cout << "\t\033[33m" << idx << "\t" << e.st->num << "\t" << e.st->c << "\t"
            << *e.st << "\t" << (e.next) << "\033[0m\n";
        }

        void display_dfa() const
        {
            RangeList * l = this->st_range_list;
            for (; l < this->st_range_list_last && l->first != l->last; ++l) {
                std::cout << l << "  st: " << l->st->num << (l->st->is_cap() ? " (cap)\n" : "\n");
                for (StateList * first = l->first, * last = l->last; first != last; ++first) {
                    std::cout << "\t" << first->st->num << "\t" << first->st->c << "\t"
                    << *first->st << "\t" << first->next << ("\n");
                }
            }
            std::cout << std::endl;
        }

    private:
        struct Matching
        {
            StateMachine2 &sm;
            unsigned step_id;

            Matching(StateMachine2& sm)
            : sm(sm)
            , step_id(1)
            {}

            typedef StateListByStep::Info Info;

            struct CompareInfoId {
                bool operator()(const Info& a, const Info& b) const {
                    return a.rl->st->num >= b.rl->st->num;
                }
            };

            unsigned step(const char *s, StateListByStep * l1, StateListByStep * l2)
            {
                std::sort(l1->begin(), l1->end(), CompareInfoId());

                for (Info* ifirst = l1->begin(), * ilast = l1->end(); ifirst != ilast ; ++ifirst) {
                    if (ifirst->rl->st->id == this->step_id) {
                        /**///std::cout << "\t\033[35mx " << (ifirst->idx) << "\033[0m\n";
                        this->sm.push_idx_trace(ifirst->idx);
                        continue;
                    }

                    unsigned new_trace = 0;
                    ifirst->rl->st->id = this->step_id;
                    StateList * first = ifirst->rl->first;
                    StateList * last = ifirst->rl->last;

                    for (; first != last; ++first) {
                        if (first->st->is_cap_open()) {
                            if (!this->sm.traces[ifirst->idx * (this->sm.nb_capture * 2) + first->st->num]) {
#ifdef DISPLAY_TRACE
                                std::cout << ifirst->idx << "  " << *first->st << "  " << first->st->num << std::endl;
#endif
                                ++this->sm.traces[ifirst->idx * (this->sm.nb_capture * 2) + first->st->num] = s;

                            }
                            continue ;
                        }

                        if (first->st->is_cap_close()) {
#ifdef DISPLAY_TRACE
                            std::cout << ifirst->idx << "  " << *first->st << "  " << first->st->num << std::endl;
#endif
                            ++this->sm.traces[ifirst->idx * (this->sm.nb_capture * 2) + first->st->num] = s;
                            continue ;
                        }

                        if (first->st->check(*s)) {
#ifdef DISPLAY_TRACE
                            this->sm.display_elem_state_list(*first, ifirst->idx);
#endif

                            if (0 == first->next) {
                                /**///std::cout << "idx: " << (ifirst->idx) << std::endl;
                                return ifirst->idx;
                            }

                            const unsigned idx = (0 == new_trace)
                                ? ifirst->idx
                                : this->sm.pop_idx_trace(ifirst->idx);
#ifdef DISPLAY_TRACE
                            std::cout << "\t\033[32m" << ifirst->idx << " -> " << idx << "\033[0m" << std::endl;
#endif
                            l2->push_back(first->next, idx);
                            ++new_trace;
                        }
                    }
                    if (0 == new_trace) {
#ifdef DISPLAY_TRACE
                        std::cout << "\t\033[35mx " << ifirst->idx << std::endl;
#endif
                        this->sm.push_idx_trace(ifirst->idx);
                    }
                }

                return -1u;
            }

            bool exact_match(const char * s)
            {
#ifdef DISPLAY_TRACE
                this->sm.display_dfa();
#endif

                this->sm.l1.clear();
                this->sm.l2.clear();
                this->sm.reset_trace();

                StateListByStep * pal1 = &this->sm.l1;
                StateListByStep * pal2 = &this->sm.l2;
                pal1->push_back(this->sm.st_range_list, *--this->sm.pidx_trace_free);

                for(; *s; ++s){
#ifdef DISPLAY_TRACE
                    std::cout << "\033[01;31mc: '" << *s << "'\033[0m" << std::endl;
#endif
                    if (-1u != (this->sm.idx_trace = this->step(s, pal1, pal2))) {
                        //this->sm.idx_trace = 36;
                        return !*(s+1);
                    }
                    ++this->step_id;
                    std::swap<>(pal1, pal2);
                    pal2->clear();
                }

                return false;
            }
        };

        friend class Matching;
        friend class Searching;

        typedef std::vector<StateBase*> state_list;
        typedef state_list::iterator state_iterator;

        unsigned nb_capture;
        unsigned idx_trace;
        unsigned * idx_trace_free;
        unsigned * pidx_trace_free;
        StateBase * st_first;
        StateBase ** captures;
        StateBase ** pcaptures;
        StateBase ** captures_for_list;
        StateBase ** pcaptures_for_list;
        const char ** traces;
        state_list vec;
        StateListByStep l1;
        StateListByStep l2;
    };


    State * state(State * out1, State * out2 = 0) {
        return new State(SPLIT, out1, out2);
    }

    State * single(int c, State * out = 0) {
        return new State(c, out);
    }

    State * any(State * out = 0) {
        return new State(ANY_CHARACTER, out);
    }

    State * zero_or_more(int c, State * out = 0) {
        State * ret = state(single(c), out);
        ret->out1->out1 = ret;
        return ret;
    }

    State * zero_or_one(int c, State * out = 0) {
        return state(single(c, out), out);
    }

    State * one_or_more(int c, State * out = 0) {
        return zero_or_more(c, out)->out1;
    }

    int c2rgxc(int c)
    {
        switch (c) {
            case 'n': return '\n';
            case 't': return '\t';
            case 'r': return '\r';
            case 'v': return '\v';
            default : return c;
        }
    }

    const char * check_interval(int a, int b)
    {
        bool valid = ('0' <= a && a <= '9' && '0' <= b && b <= '9')
        || ('a' <= a && a <= 'z' && 'a' <= b && b <= 'z')
        || ('A' <= a && a <= 'Z' && 'A' <= b && b <= 'Z');
        return (valid && a <= b) ? 0 : "range out of order in character class";
    }

    StateBase * str2stchar(const char *& s, const char * last)
    {
        if (*s == '\\' && s+1 != last) {
            return new State(c2rgxc(*++s));
        }

        if (*s == '[') {
            StateMultiTest * st = new StateMultiTest;
            std::string str;
            if (++s != last && *s != ']') {
                if (*s == '-') {
                    str += '-';
                    ++s;
                }
                const char * c = s;
                while (s != last && *s != ']') {
                    const char * p = s;
                    while (++s != last && *s != ']' && *s != '-') {
                        if (*s == '\\' && s+1 != last) {
                            str += c2rgxc(*++s);
                        }
                        else {
                            str += *s;
                        }
                    }

                    if (*s == '-') {
                        if (c == s) {
                            str += '-';
                        }
                        else if (s+1 == last) {
                            std::cerr << "missing terminating ]" << std::endl; _exit(0); //TODO
                        }
                        else if (*(s+1) == ']') {
                            str += '-';
                            ++s;
                        }
                        else if (s == p) {
                            str += '-';
                        }
                        else {
                            if (const char * err = check_interval(*(s-1), *(s+1))) {
                                std::cerr << err << std::endl; _exit(0); //TODO
                            }
                            if (str.size() > 1) {
                                str.erase(str.size()-1);
                            }
                            st->push_checker(new CheckerInterval(*(s-1), *(s+1)));
                            c = ++s + 1;
                        }
                    }
                }
            }

            if (!str.empty()) {
                st->push_checker(new CheckerString(str));
            }

            if (*s != ']') {
                std::cerr << "missing terminating ]" << std::endl; _exit(0); //TODO
                delete st;
                st = 0;
            }

            return st;
        }

        return new State(*s == '.' ? ANY_CHARACTER : *s);
    }

    bool is_meta_char(int c)
    {
        return c == '*' || c == '+' || c == '?' || c == '|' || c == '(' || c == ')';
    }

    typedef std::pair<StateBase*, StateBase**> IntermendaryState;

    IntermendaryState intermendary_str2reg(const char *& s, const char * last, int recusive = 0)
    {
        StateBase st(0);
        StateBase ** pst = &st.out2;
        StateBase * bst = &st;

        StateBase ** besplit[50] = {0};
        StateBase *** pesplit = besplit;

        while (s != last) {
            if (!is_meta_char(*s)) {
                *pst = str2stchar(s, last);
                while (++s != last && !is_meta_char(*s)) {
                    pst = &(*pst)->out1;
                    *pst = str2stchar(s, last);
                }
            }
            else {
                switch (*s) {
                    case '?':
                        *pst = new State(SPLIT, *pst);
                        pst = &(*pst)->out2;
                        break;
                    case '*':
                        *pst = new State(SPLIT, *pst);
                        (*pst)->out1->out1 = *pst;
                        pst = &(*pst)->out2;
                        break;
                    case '+':
                        (*pst)->out1 = new State(SPLIT, *pst);
                        pst = &(*pst)->out1->out2;
                        break;
                    case '|':
                        *pesplit = pst;
                        ++pesplit;
                        bst->out2 = new State(SPLIT, bst->out2);
                        bst = bst->out2;
                        pst = &bst->out2;
                        break;
                    case ')':
                        if (0 == recusive) {
                            std::cerr << "unmatched parentheses" << std::endl; _exit(0); //TODO
                        }
                        if (*pst) {
                            pst = &(*pst)->out1;
                        }
                        *pst = new State(CAPTURE_CLOSE);
                        for (StateBase *** first = besplit; first != pesplit; ++first) {
                            (**first)->out1 = *pst;
                        }
                        return IntermendaryState(st.out2, &(*pst)->out1);
                        break;
                    default:
                        //TODO impossible
                        std::cout << ("error") << std::endl;
                        break;
                    case '(':
                        IntermendaryState intermendary = intermendary_str2reg(++s, last, recusive+1);
                        if (intermendary.first) {
                            if (*pst) {
                                pst = &(*pst)->out1;
                            }
                            *pst = new State(CAPTURE_OPEN, intermendary.first);
                            pst = intermendary.second;
                        }
                        break;
                }
                ++s;
            }
        }
        if (0 != recusive) {
            std::cerr << "unmatched parentheses" << std::endl; _exit(0); //TODO
        }
        return IntermendaryState(st.out2, pst);
    }

    StateBase* str2reg(const char * s, const char * last)
    {
        return intermendary_str2reg(s, last).first;
    }

    StateBase* str2reg(const char * s)
    {
        return str2reg(s, s + strlen(s));
    }

    void display_state(StateBase * st, unsigned depth = 0)
    {
        if (st && st->id != -1u-1u) {
            std::string s(depth, '\t');
            std::cout
            << s << "\033[33m" << st << "\t" << st->num << "\t" << st->c << "\t"
            << *st << "\033[0m\n\t" << s << st->out1 << "\n\t" << s << st->out2 << "\n";
            st->id = -1u-1u;
            display_state(st->out1, depth+1);
            display_state(st->out2, depth+1);
        }
    }
}

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);

    using namespace rndfa;

#ifdef GENERATE_ST
    if (argc < 2) {
        std::cerr << argv[0] << (" regex") << std::endl;
    }
    const char * rgxstr = argv[1];
    StateBase * st = str2reg(argv[1]);
    display_state(st);
    std::cout.flush();

    if (argc < 3) {
        return 0;
    }
    else {
        argv[1] = argv[2];
    }
#else

    //State last('\0');
//     State last(ANY_CHARACTER);
//     StateRange digit('0', '9');
// //     State& last = digit;
//     State one_more(SPLIT, &digit, &last);
//     digit.out1 = &one_more;
    //(ba*b?a|a*b[uic].*s)[0-9]
//     State * st = state(
//         single('b',
//                zero_or_more('a',
//                             zero_or_one('b',
//                                         single('a',
//                                                &digit)))),
//         zero_or_more('a',
//                      single('b',
//                             characters("uic",
//                                        zero_or_more(ANY_CHARACTER,
//                                                     single('s',
//                                                            &digit)))))
//     );
    //b(a*b?a|[uic].*s)[0-9]*
//     State * st =
//         single('b',
//                state(
//                    zero_or_one('a',
//                                zero_or_one('b',
//                                            single('a',
//                                                   &digit)
//                                           )
//                               ),
//                      characters("uic",
//                                 zero_or_more(ANY_CHARACTER,
//                                              single('s',
//                                                     &digit)
//                                              )
//                                 )
//                      )
//                );

    State char_a('a');

    State * st = zero_or_more(
        ANY_CHARACTER,
        single(
            ' ',
            zero_or_more(
                ANY_CHARACTER,
                single(
                    ' ',
                    single(
                        CAPTURE_OPEN,
                        zero_or_more(
                            ANY_CHARACTER,
                            single(
                                CAPTURE_CLOSE,
                                single(
                                    ' ',
                                    single(
                                        CAPTURE_OPEN,
                                        zero_or_more(
                                            ANY_CHARACTER,
                                            single(
                                                CAPTURE_CLOSE,
                                                single(
                                                    ' ',
                                                    zero_or_more(
                                                        ANY_CHARACTER,
                                                        &char_a
                                                    )
                                                )
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    );
#endif

    typedef StateMachine2 Regex;

    Regex sm(st);

    //display_state(st);

    regex_t rgx;
#ifdef GENERATE_ST
    if (0 != regcomp(&rgx, rgxstr, REG_EXTENDED)){
#else
    if (0 != regcomp(&rgx, ".* .* (.*) (.*) .*a.*", REG_EXTENDED)){
#endif
        std::cout << ("comp error") << std::endl;
    }
    regmatch_t regmatch[3];

    bool ismatch1 = false;
    bool ismatch2 = false;
    bool ismatch3 = false;
    bool ismatch4 = false;
    double d1, d2, d3, d4;

    const char * str = argc > 1 ? argv[1] : "abcdef";

#ifndef ITERATION
# define ITERATION 100000
#endif
    {
        regexec(&rgx, str, 1, regmatch, 0); //NOTE preload
        //BEGIN
        std::clock_t start_time = std::clock();
        for (size_t i = 0; i < ITERATION; ++i) {
            ismatch1 = 0 == regexec(&rgx, str, 1, regmatch, 0);
        }
        d1 = double(std::clock() - start_time) / CLOCKS_PER_SEC;
        //END
    }
    {
        std::clock_t start_time = std::clock();
        for (size_t i = 0; i < ITERATION; ++i) {
            ismatch2 = sm.exact_search(str);
        }
        d2 = double(std::clock() - start_time) / CLOCKS_PER_SEC;
    }
    //std::streambuf * dbuf = std::cout.rdbuf(0);
    {
        struct test {
            inline static bool
            impl(regex_t& rgx, const char * s, regmatch_t * m, unsigned size) {
                const char * str = s;
                while (0 == regexec(&rgx, s, size, m, 0)) {
                    for (unsigned i = 1; i < size; i++) {
                        if (m[i].rm_so == -1) {
                            break;
                        }
                        int start = m[i].rm_so + (s - str);
                        int finish = m[i].rm_eo + (s - str);
                        //std::cout.write(str+start, finish-start) << "\n";
                    }
                    s += m[0].rm_eo;
                }
                return 0 == *s;
            }
        };
        test::impl(rgx, str, regmatch, sizeof(regmatch)/sizeof(regmatch[0])); //NOTE preload
        //BEGIN
        std::clock_t start_time = std::clock();
        for (size_t i = 0; i < ITERATION; ++i) {
            ismatch3 = test::impl(rgx, str, regmatch, sizeof(regmatch)/sizeof(regmatch[0]));
        }
        d3 = double(std::clock() - start_time) / CLOCKS_PER_SEC;
        //END
    }
    {
        std::clock_t start_time = std::clock();
        for (size_t i = 0; i < ITERATION; ++i) {
            ismatch4 = sm.exact_search_with_trace(str);
            Regex::range_list match_result = sm.match_result();
            typedef Regex::range_list::iterator iterator;
            for (iterator first = match_result.begin(), last = match_result.end(); first != last; ++first) {
                //std::cout.write(str+first->first, first->second) << "\n";
            }
        }
        d4 = double(std::clock() - start_time) / CLOCKS_PER_SEC;
    }
    //std::cout.rdbuf(dbuf);

    std::cout.precision(2);
    std::cout.setf(std::ios::fixed);
    std::cout
#if GENERATE_ST
    << "regex: " << rgxstr << "\n"
#else
    << "regex: '.* .* (.*) (.*) .*a'\n"
#endif
    << "search:\n"
    << (ismatch1 ? "good\n" : "fail\n")
    << d1 << " sec\n"
    << (ismatch2 ? "good\n" : "fail\n")
    << d2 << " sec\n"
    << "match:\n"
    << (ismatch3 ? "good\n" : "fail\n")
    << d3 << " sec\n"
    << (ismatch4 ? "good\n" : "fail\n")
    << d4 << " sec\n"
    << std::endl;

    if (ismatch3) {
        std::cout << ("with regex.h\n");
        for (unsigned i = 1; i < sizeof(regmatch)/sizeof(regmatch[0]); i++) {
            if (regmatch[i].rm_so == -1) {
                break;
            }
            int start = regmatch[i].rm_so;
            int finish = regmatch[i].rm_eo;
            (std::cout << "\tmatch: '").write(str+start, finish-start) << "'\n";
        }
    }
    if (ismatch4) {
        std::cout << ("with dfa\n");
        Regex::range_list match_result = sm.match_result();
        typedef Regex::range_list::iterator iterator;
        for (iterator first = match_result.begin(), last = match_result.end(); first != last; ++first) {
            (std::cout << "\tmatch: '").write(first->first, first->second-first->first) << "'\n";
        }
        std::cout.flush();
    }
    regfree(&rgx);
}
