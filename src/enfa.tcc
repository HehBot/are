#ifndef ENFA_TCC
#define ENFA_TCC

#include <fstream>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template<typename a, class a_Hash> class enfa;

template<typename Alpha, class Alpha_Hash = std::hash<Alpha>>
class nfa {
protected:
    std::size_t Q; // state space

    std::size_t q0;
    std::set<std::size_t> F;

    std::vector<Alpha> Sigma; // alphabet
    std::unordered_map<Alpha, std::size_t, Alpha_Hash> Sigma_index;

    std::vector<std::vector<std::set<std::size_t>>> delta;

protected:
    virtual void add_state(std::size_t q)
    {
        if (q >= Q) {
            for (std::size_t i = Q; i <= q; ++i)
                delta.push_back(std::vector<std::set<std::size_t>>(Sigma.size()));
            Q = q + 1;
        }
    }
    virtual void add_letter(Alpha a)
    {
        if (Sigma_index.count(a) == 0) {
            Sigma_index[a] = Sigma.size();
            Sigma.push_back(a);
            std::size_t S = Sigma.size();
            for (std::size_t i = 0; i < Q; ++i)
                delta[i].resize(S);
        }
    }

    virtual void print_aux(std::ostream& f, std::size_t q) const
    {
    }

public:
    nfa()
        : Q(0), q0(-1)
    {
    }

    void add_transition(std::size_t q1, Alpha a, std::size_t q2)
    {
        add_state(q1);
        add_state(q2);
        add_letter(a);

        delta[q1][Sigma_index[a]].insert(q2);
    }
    void mark_start_state(std::size_t q)
    {
        add_state(q);
        q0 = q;
    }
    void mark_finish_state(std::size_t q)
    {
        add_state(q);
        F.insert(q);
    }

    virtual bool accept(std::vector<Alpha> const& str)
    {
        if (q0 + 1 == 0)
            throw std::domain_error("\u03b5-NFA start state not defined");
        if (F.size() == 0)
            return false;

        std::set<std::size_t> curr = { q0 };

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
            curr = temp;
        }

        for (std::size_t qf : F)
            if (curr.count(qf) == 1)
                return true;
        return false;
    }

    void create_dot_file(char const* filename) const
    {
        std::ofstream f(filename);

        f << "digraph {\n";
        if (q0 + 1 != 0) {
            f << "\tn0 [label=\"\", shape=none, height=0, width=0]\n";
            f << "\tn0 -> " << q0 << " [shape=doublecircle]\n";
        }
        for (std::size_t qf : F)
            f << "\t" << qf << " [shape=doublecircle]\n";
        for (std::size_t q1 = 0; q1 < Q; q1++) {
            print_aux(f, q1);
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

    friend class enfa<Alpha, Alpha_Hash>;
};

// epsilon-nfa with single start state and atmost 1 finish state
template<typename Alpha, class Alpha_Hash = std::hash<Alpha>>
class enfa : public nfa<Alpha, Alpha_Hash> {
    using nfa<Alpha, Alpha_Hash>::Q;
    using nfa<Alpha, Alpha_Hash>::q0;
    using nfa<Alpha, Alpha_Hash>::F;
    using nfa<Alpha, Alpha_Hash>::Sigma;
    using nfa<Alpha, Alpha_Hash>::Sigma_index;
    using nfa<Alpha, Alpha_Hash>::delta;

protected:
    std::vector<std::set<std::size_t>> epsilon_closure; // contains epsilon closure of state q
    std::vector<std::set<std::size_t>> epsilon_parents; // contains epsilon parents of state q

protected:
    virtual void add_state(std::size_t q) override
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
    void update_epsilon_closure(std::size_t q, std::set<std::size_t> const& e_c, std::set<std::size_t>& visited)
    {
        if (visited.count(q) == 1)
            return;
        visited.insert(q);
        epsilon_closure[q].insert(e_c.begin(), e_c.end());
        for (std::size_t qp : epsilon_parents[q])
            update_epsilon_closure(qp, e_c, visited);
    }

    virtual void print_aux(std::ostream& f, std::size_t q) const override
    {
        for (std::size_t qp : epsilon_parents[q])
            f << "\t" << qp << " -> " << q << " [label=\"\u03b5\"]\n";
    }

public:
    enfa()
        : nfa<Alpha, Alpha_Hash>()
    {
    }

    void add_epsilon_transition(std::size_t q1, std::size_t q2)
    {
        add_state(q1);
        add_state(q2);

        if (q1 == q2)
            return;

        epsilon_parents[q2].insert(q1);

        std::set<std::size_t> visited;
        update_epsilon_closure(q1, epsilon_closure[q2], visited);
    }

    virtual bool accept(std::vector<Alpha> const& str) override
    {
        if (q0 + 1 == 0)
            throw std::domain_error("\u03b5-NFA start state not defined");
        if (F.size() == 0)
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

        for (std::size_t qf : F)
            if (curr.count(qf) == 1)
                return true;
        return false;
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

        e1.add_epsilon_transition(new_q0, e1.q0);
        e1.add_epsilon_transition(new_q0, new_qf);
        for (std::size_t e1_qf : e1.F) {
            e1.add_epsilon_transition(e1_qf, e1.q0);
            e1.add_epsilon_transition(e1_qf, new_qf);
        }

        e1.q0 = new_q0;
        e1.F = { new_qf };

        return e1;
    }

public:
    static enfa<Alpha> concat_enfa(enfa<Alpha> e1, enfa<Alpha> const& e2)
    {
        std::size_t e1_qf;
        if (e1.F.size() == 1)
            e1_qf = *e1.F.begin();
        else {
            e1_qf = e1.Q;
            e1.add_state(e1_qf);
            for (std::size_t q : e1.F)
                e1.add_epsilon_transition(q, e1_qf);
            e1.F = { e1_qf };
        }

        std::vector<std::size_t> e2_to_new_e1(e2.Q, -1);
        for (std::size_t q = 0; q < e2.Q; ++q) {
            std::size_t q_new;
            if (q == e2.q0) {
                q_new = e1_qf;
            } else {
                q_new = e1.Q;
                e1.add_state(e1.Q);
            }
            e2_to_new_e1[q] = q_new;
        }
        for (std::size_t q1 = 0; q1 < e2.Q; ++q1) {
            for (std::size_t a = 0; a < e2.Sigma.size(); ++a)
                for (std::size_t q2 : e2.delta[q1][a])
                    e1.add_transition(e2_to_new_e1[q1], e2.Sigma[a], e2_to_new_e1[q2]);
        }

        for (std::size_t q2 = 0; q2 < e2.Q; ++q2) {
            std::set<std::size_t> new_e_c, new_e_p;
            for (std::size_t qc : e2.epsilon_closure[q2])
                new_e_c.insert(e2_to_new_e1[qc]);
            for (std::size_t qp : e2.epsilon_parents[q2])
                new_e_p.insert(e2_to_new_e1[qp]);
            e1.epsilon_closure[e2_to_new_e1[q2]].insert(new_e_c.begin(), new_e_c.end());
            e1.epsilon_parents[e2_to_new_e1[q2]].insert(new_e_p.begin(), new_e_p.end());
        }

        for (std::size_t e1_qf : e1.F)
            e1.add_epsilon_transition(e1_qf, e2_to_new_e1[e2.q0]);

        e1.F.clear();
        for (std::size_t e2_qf : e2.F)
            e1.F.insert(e2_to_new_e1[e2_qf]);

        return e1;
    }
    static enfa<Alpha> union_enfa(enfa<Alpha> e1, enfa<Alpha> const& e2)
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

        std::size_t new_q0 = e1.Q;
        std::size_t new_qf = e1.Q + 1;

        e1.add_epsilon_transition(new_q0, e1.q0);
        for (std::size_t e1_qf : e1.F)
            e1.add_epsilon_transition(e1_qf, new_qf);
        e1.add_epsilon_transition(new_q0, e2.q0 + inc);
        for (std::size_t e2_qf : e2.F)
            e1.add_epsilon_transition(e2_qf + inc, new_qf);

        e1.q0 = new_q0;
        e1.F = { new_qf };

        return e1;
    }

    nfa<Alpha> to_nfa() const
    {
        nfa<Alpha> n;

        n.Sigma = Sigma;
        n.Sigma_index = Sigma_index;
        n.add_state(Q - 1);
        n.mark_start_state(q0);
        n.F = F;
        for (std::size_t q : epsilon_closure[q0]) {
            if (F.count(q) == 1) {
                n.F.insert(q0);
                break;
            }
        }

        for (std::size_t q = 0; q < Q; ++q)
            for (std::size_t a = 0; a < Sigma.size(); ++a)
                for (std::size_t r : epsilon_closure[q])
                    for (std::size_t d : delta[r][a]) {
                        auto const& ec = epsilon_closure[d];
                        n.delta[q][a].insert(ec.begin(), ec.end());
                    }

        return n;
    }
};

#endif // ENFA_TCC
