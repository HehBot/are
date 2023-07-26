#ifndef ENFA_TCC
#define ENFA_TCC

#include <fstream>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// epsilon-nfa with single start state and atmost 1 finish state
template<typename Alpha, class Alpha_Hash = std::hash<Alpha>>
class enfa {
private:
    std::size_t Q; // state space

    std::vector<Alpha> Sigma; // alphabet
    std::unordered_map<Alpha, std::size_t, Alpha_Hash> Sigma_index;

    std::size_t q0, qf;

    std::vector<std::vector<std::set<std::size_t>>> delta;
    std::vector<std::set<std::size_t>> epsilon_closure; // contains epsilon closure of state q
    std::vector<std::set<std::size_t>> epsilon_parents; // contains epsilon parents of state q

private:
    void add_state(std::size_t q)
    {
        if (q >= Q) {
            epsilon_closure.resize(q + 1);
            epsilon_parents.resize(q + 1);
            for (std::size_t i = Q; i <= q; ++i) {
                delta.push_back(std::vector<std::set<std::size_t>>(Sigma.size()));
                epsilon_closure[i] = { i };
            }
            Q = q + 1;
        }
    }
    void add_letter(Alpha a)
    {
        if (Sigma_index.count(a) == 0) {
            Sigma_index[a] = Sigma.size();
            Sigma.push_back(a);
            std::size_t S = Sigma.size();
            for (std::size_t i = 0; i < Q; ++i)
                delta[i].resize(S);
        }
    }
    void update_epsilon_closure(std::size_t q, std::set<std::size_t> const& e_c, std::set<std::size_t>& visited)
    {
        if (visited.count(q) == 1)
            return;
        visited.insert(q);
        epsilon_closure[q].insert(e_c.begin(), e_c.end());
        for (std::size_t qp : epsilon_parents[q])
            update_epsilon_closure(qp, e_c, visited);
    }

public:
    enfa()
        : Q(0), q0(-1), qf(-1)
    {
    }

    void add_transition(std::size_t q1, Alpha a, std::size_t q2)
    {
        add_state(q1);
        add_state(q2);
        add_letter(a);

        delta[q1][Sigma_index[a]].insert(q2);
    }
    void add_epsilon_transition(std::size_t q1, std::size_t q2)
    {
        add_state(q1);
        add_state(q2);

        epsilon_parents[q2].insert(q1);

        std::set<std::size_t> visited;
        update_epsilon_closure(q1, epsilon_closure[q2], visited);
    }
    void mark_start_state(std::size_t q)
    {
        add_state(q);
        q0 = q;
    }
    void mark_finish_state(std::size_t q)
    {
        add_state(q);
        qf = q;
    }

    bool accept(std::vector<Alpha> const& str)
    {
        if (q0 + 1 == 0)
            throw std::domain_error("\u03b5-NFA start state not defined");
        if (qf + 1 == 0)
            return false;

        std::set<std::size_t> curr = epsilon_closure[q0];

        for (auto const& alpha : str) {
            // temp = delta(curr, alpha)
            if (Sigma_index.count(alpha) == 0)
                return false;
            std::size_t a = Sigma_index[alpha];
            std::set<std::size_t> temp;
            for (std::size_t q : curr) {
                auto const& s = delta[q][a];
                temp.insert(s.begin(), s.end());
            }

            // curr = epsilon_closure(temp)
            curr.clear();
            for (std::size_t q : temp) {
                auto const& e_c = epsilon_closure[q];
                curr.insert(e_c.begin(), e_c.end());
            }
        }

        return (curr.count(qf) == 1);
    }

    void create_dot_file(char const* filename) const
    {
        std::ofstream f(filename);

        f << "digraph {\n";
        if (q0 + 1 != 0) {
            f << "\tn0 [label=\"\", shape=none, height=0, width=0]\n";
            f << "\tn0 -> " << q0 << " [shape=doublecircle]\n";
        }
        if (qf + 1 != 0)
            f << "\t" << qf << " [shape=doublecircle]\n";
        for (std::size_t q1 = 0; q1 < Q; q1++) {
            for (std::size_t qp : epsilon_parents[q1])
                f << "\t" << qp << " -> " << q1 << " [label=\"\u03b5\"]\n";
            for (std::size_t a = 0; a < Sigma.size(); ++a) {
                auto const& s = delta[q1][a];
                auto it = s.begin();
                if (it != s.end()) {
                    f << "\t" << q1 << " -> ";
                    for (; std::next(it) != s.end(); ++it)
                        f << *it << ",";
                    f << *it << " [label=\"" << Sigma[a] << "\"]\n";
                }
            }
        }
        f << "}\n";
    }

    static enfa<Alpha> empty_expr_enfa()
    {
        enfa<Alpha> e;
        e.mark_start_state(0);
        e.mark_finish_state(1);
        e.add_epsilon_transition(0, 1);
        return e;
    }
    static enfa<Alpha> letter_enfa(Alpha a)
    {
        enfa<Alpha> e;
        e.mark_start_state(0);
        e.mark_finish_state(1);
        e.add_transition(0, a, 1);
        return e;
    }
    static enfa<Alpha> kleene_star_enfa(enfa<Alpha> e1)
    {
        std::size_t new_q0 = e1.Q;
        std::size_t new_qf = e1.Q + 1;

        e1.add_epsilon_transition(e1.qf, e1.q0);
        e1.add_epsilon_transition(new_q0, e1.q0);
        e1.add_epsilon_transition(e1.qf, new_qf);
        e1.add_epsilon_transition(new_q0, new_qf);

        e1.q0 = new_q0;
        e1.qf = new_qf;

        return e1;
    }

private:
    static std::size_t fuse_enfa(enfa<Alpha>& e1, enfa<Alpha> const& e2)
    {
        std::size_t inc = e1.Q;

        for (std::size_t q1 = 0; q1 < e2.Q; ++q1) {
            e1.add_state(q1 + inc);
            for (std::size_t a = 0; a < e2.Sigma.size(); ++a)
                for (std::size_t q2 : e2.delta[q1][a])
                    e1.add_transition(q1 + inc, e2.Sigma[a], q2 + inc);
        }

        for (std::size_t q2 = 0; q2 < e2.Q; ++q2) {
            std::set<std::size_t> new_e_c, new_e_p;
            for (std::size_t qc : e2.epsilon_closure[q2])
                new_e_c.insert(qc + inc);
            for (std::size_t qp : e2.epsilon_parents[q2])
                new_e_p.insert(qp + inc);
            e1.epsilon_closure[q2 + inc] = new_e_c;
            e1.epsilon_parents[q2 + inc] = new_e_p;
        }

        return inc;
    }

public:
    static enfa<Alpha> concat_enfa(enfa<Alpha> e1, enfa<Alpha> const& e2)
    {
        std::size_t inc = fuse_enfa(e1, e2);

        e1.add_epsilon_transition(e1.qf, e2.q0 + inc);
        e1.qf = e2.qf + inc;

        return e1;
    }
    static enfa<Alpha> union_enfa(enfa<Alpha> e1, enfa<Alpha> const& e2)
    {
        std::size_t inc = fuse_enfa(e1, e2);

        std::size_t new_q0 = e1.Q;
        std::size_t new_qf = e1.Q + 1;

        e1.add_epsilon_transition(new_q0, e1.q0);
        e1.add_epsilon_transition(e1.qf, new_qf);
        e1.add_epsilon_transition(new_q0, e2.q0 + inc);
        e1.add_epsilon_transition(e2.qf + inc, new_qf);

        e1.q0 = new_q0;
        e1.qf = new_qf;

        return e1;
    }
};

#endif // ENFA_TCC
