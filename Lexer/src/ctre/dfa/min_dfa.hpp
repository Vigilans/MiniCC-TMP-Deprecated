#ifndef MIN_DFA_H_
#define MIN_DFA_H_
#include "dfa.hpp"

namespace cp {

/* ---------------------min_dfa�㷨ʵ��--------------------- */

template <class DFA> 
struct _min_dfa_impl {

    // ����ϸ�ֽ׶�
    template <class... Groups> // �洢һ�����ֵĸ�С����б�
    using division = group_list<std::tuple<Groups...>, std::is_same, empty_group>;

    template <class Division, bool recursive>
    struct split_to_atom; // ����ϸ�ֻ��֣�ֱ��������ϸ��Ϊֹ
    template <class Division>
    struct split_to_atom<Division, false> { using result = Division; }; // ֹͣ�ݹ�ʱ��ֱ�ӷ��ص�ǰ����
    template <class... Groups>
    struct split_to_atom<division<Groups...>, true> { // ���ݹ����
        using cur_division = division<Groups...>;

        // �ɵ�ǰ���ֲ����µĻ���
        template <class... Groups>
        struct split_division;
        template <>
        struct split_division<> { using result = division<>; };
        template <class ThisGroup, class... RestGroups>
        struct split_division<ThisGroup, RestGroups...> {
            // �� pair<Դ״̬ --(�ַ���)--> Ŀ�꼯��> �����еȡ�ֻ�ж�ӳ���Ŀ�꼯���Ƿ���ȡ���ȵ�Pair�ᱻ����һ��
            template <class Left, class Right>
            struct type_pair_group_equal : std::is_same<typename Left::second, typename Right::second> {};

            template <class... Pairs> // �洢pair<Դ״̬ --(�ַ���)--> Ŀ�꼯��>���б�
            using pair_list = group_list<std::tuple<Pairs...>, type_pair_group_equal, type_pair<null_state, empty_group>>;

            // ��ȡGroup������״̬��pair<Դ״̬ --(�ַ���)--> Ŀ�꼯��>��ӳ���б�
            template <class GroupTuple> 
            struct map;
            template <> 
            struct map<std::tuple<>> { using result = pair_list<>; };
            template <class ThisState, class... RestStates>
            struct map<std::tuple<ThisState, RestStates...>> {
                template <class char_set> struct map_charset;
                template <char... chs> struct map_charset<char_set<char_sequence<chs...>>> {
                    using result = state_group<typename DFA::template trans<ThisState, chs>...>; // ����û��ʹ��state_group���������ʣ���˳��������
                };

                using this_mapped_pair = type_pair<ThisState, typename map_charset<typename DFA::charset>::result>;
                using rest_mapped_pairs = typename map<std::tuple<RestStates...>>::result;
                using result = typename rest_mapped_pairs::template insert<this_mapped_pair>;
            };

            // ��pair<Դ״̬ --(�ַ���)--> Ŀ�꼯��>�б�ѹ�����µļ���С�飬�õ��µķֻ���
            template <class PairList>
            struct reduce;
            template <>
            struct reduce<pair_list<>>     { using result = division<>; };
            template <class Pair>
            struct reduce<pair_list<Pair>> { using result = division<state_group<typename Pair::first>>; };
            template <class ThisPair, class Neighbor, class... RestPairs>
            struct reduce<pair_list<ThisPair, Neighbor, RestPairs...>> {
                using rest_groups = typename reduce<pair_list<Neighbor, RestPairs...>>::result;
                using near_merged = typename rest_groups::front::template insert<typename ThisPair::first>; // ���ھӺϲ�
                using independent = typename state_group<typename ThisPair::first>; // ��ǰ״̬�Գ�һ��
                using result = std::conditional_t<
                    std::is_same_v<typename ThisPair::second, typename Neighbor::second>,
                    typename division<near_merged>::template concat<typename rest_groups::rest>, // ��Ŀ��״̬������ȣ�����������
                    typename division<independent>::template concat<rest_groups> // �����½�һ����
                >;
            };

            using new_this_group  = typename reduce<typename map<typename ThisGroup::reverse::tuple>::result>::result; // reverse::tuple��Ϊ���������
            using new_rest_groups = typename split_division<RestGroups...>::result;
            using result = typename new_this_group::template concat<new_rest_groups>;
        };

        using new_division = typename split_division<Groups...>::result;

        // ���»�����ԭ�������ʱ��ֹͣ�ݹ�
        using result = typename split_to_atom<new_division, !std::is_same_v<new_division, cur_division>>::result;
    };

    // ��ȡ���յĲ�����ϸ�ֵĻ��֣���ʼ����ΪS-F��F��
    using final_division = typename split_to_atom<division<typename DFA::normal_states, typename DFA::accept_states>, true>::result;

    // ӳ��׶Σ�ӳ�������һ�����״̬
    template <class ListOrGroup> struct map_rep;
    template <> struct map_rep<division<>>  { using result = empty_group; };
    template <> struct map_rep<empty_group> { using result = empty_group; };
    template <class Group, class... Rest> struct map_rep<division<Group, Rest...>> {    // ӳ��List��ÿ��Group�����ǵĴ���״̬
        using result = typename map_rep<division<Rest...>>::result::template insert<typename Group::front>; // ͬ��Ĵ���Ԫ�ز����ظ�����
    };
    template <class State, class... Rest> struct map_rep<state_group<State, Rest...>> { // ӳ��Group�е�ÿ��״̬������������Ĵ���״̬
        using rep_state = typename final_division::template first_group_of<State>::front;
        using result    = typename map_rep<state_group<Rest...>>::result::template insert<rep_state>;
    };
    
    // ������ת����׶�
    template <class... RepStates> struct set_trans_table { using result = trans_table<>; };
    template <class RepState, class... Rest> struct set_trans_table<state_group<RepState, Rest...>> {
        // ��״̬��һ���ַ���ӳ�䣬ÿ��ӳ���������ת������
        template <class char_set> struct map_charset { using result = trans_table<>; };
        template <char ch, char... rest> struct map_charset<char_set<char_sequence<ch, rest...>>> {
            using dest_state = typename DFA::template trans<RepState, ch>;
            using dest_rep   = typename final_division::template first_group_of<dest_state>::front;
            using new_trans  = transition<RepState, ch, dest_rep>;
            using result = typename map_charset<char_set<char_sequence<rest...>>>::result::template insert<new_trans>;
        };
    
        // ����transtion��������Դ״̬Ϊ��һ���ȼ�, RepStates��������ģ���ֱ��ƴ�ӵĽ��Ҳ�������
        using this_table = typename map_charset<typename DFA::charset>::result;
        using rest_table = typename set_trans_table<state_group<Rest...>>::result;
        using result = typename this_table::template concat<rest_table>;
    };
    
    // ����min_dfa�׶�
    struct build {
        using rep_initial = typename final_division::template first_group_of<typename DFA::initial_state>::front;
        using rep_accepts = typename map_rep<typename DFA::accept_states>::result;
        using rep_states  = typename map_rep<final_division>::result;
        using trans_table = typename set_trans_table<rep_states>::result;
        using result = dfa<trans_table, rep_initial, rep_accepts>;
    };
};

template <class DFA>
using min_dfa = typename _min_dfa_impl<DFA>::build::result;

}

#endif // !MIN_DFA_H_