#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...) __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...) macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define SERIALIZE_COMPONENT_IMPL(member) \
    archive &component.member;

#define SERIALIZABLE_COMPONENT(T, ...)                                              \
    namespace boost::serialization {                                                \
                                                                                    \
        template <class Archive>                                                    \
        void save(Archive &archive, T const &component, const unsigned int version) \
        {                                                                           \
            FOR_EACH(SERIALIZE_COMPONENT_IMPL, __VA_ARGS__)                         \
        }                                                                           \
                                                                                    \
        template <class Archive>                                                    \
        void load(Archive &archive, T &component, const unsigned int version)       \
        {                                                                           \
            FOR_EACH(SERIALIZE_COMPONENT_IMPL, __VA_ARGS__)                         \
        }                                                                           \
    }
