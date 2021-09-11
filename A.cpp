//  g++ -O2 -o A A.cpp && for i in $(seq 0 9); do echo ${i}; ./A < tester/input_${i}.txt > tester/output_${i}.txt; done

#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>
#include <cmath>
#include <queue>
#include <cstring>
#include <algorithm>
#include <map>
using namespace std;

const int N = 16;
const int M = 5000;
const int T = 1000;

struct Move
{
    //  購入はfrom=-1
    //  パスはto=-1
    int from, to;
    Move(): from(-1), to(-1) {}
    Move(int from, int to): from(from), to(to) {}
};

struct State
{
    short field[N*N] = {};
    bool machine[N*N] = {};
    unsigned long long hash = 0;
    long long score = 0;
    long long money = 0;
    State *prev = nullptr;
    Move move;
};

struct PosV
{
    int pos;
    int v;
    PosV(int pos, int v): pos(pos), v(v) {}
};

bool operator<(const State &s1, const State &s2)
{
    return s1.score>s2.score;
}

unsigned long long Hash[N*N][3];

vector<PosV> SPosV[T];
vector<int> EPos[T];

int xor64(void) {
    static uint64_t x = 88172645463325252ULL;
    x ^= x<<13;
    x ^= x>> 7;
    x ^= x<<17;
    return int(x&0x7fffffff);
}

//  隣接しているマシン数を返す
int calc_k_sub(bool machine[N*N], int p, vector<int> *hist);

int calc_k(bool machine[N*N], int p)
{
    assert(machine[p]);

    static vector<int> hist;
    int n = calc_k_sub(machine, p, &hist);

    while (!hist.empty())
    {
        machine[hist.back()] = true;
        hist.pop_back();
    }

    return n;
}

int calc_k_sub(bool machine[N*N], int p, vector<int> *hist)
{
    assert(machine[p]);

    machine[p] = false;
    hist->push_back(p);

    static int dr[] = {-1, 1, 0, 0};
    static int dc[] = {0, 0, -1, 1};
    int n = 1;
    for (int d=0; d<4; d++)
    {
        int tr = (p>>4) + dr[d];
        int tc = (p&15) + dc[d];
        if (0<=tr && tr<N &&
            0<=tc && tc<N &&
            machine[tr*N+tc])
            n += calc_k_sub(machine, tr*N+tc, hist);
    }
    return n;
}

long long calc_hash(short F[N*N], bool M[N*N])
{
    long long h = 0;
    for (int p=0; p<N*N; p++)
        h ^= Hash[p][(F[p]>0?2:0)+(M[p]?1:0)];
    return h;
}

int main()
{
    int _N, _M, _T;
    cin>>_N>>_M>>_T;
    for (int i=0; i<M; i++)
    {
        int R, C, S, E, V;
        cin>>R>>C>>S>>E>>V;
        SPosV[S].push_back(PosV(R*N+C, V));
        EPos[E].push_back(R*N+C);
    }

    chrono::system_clock::time_point start = chrono::system_clock::now();

    for (int p=0; p<N*N; p++)
        for (int i=0; i<3; i++)
            Hash[p][i] = (unsigned long long)xor64()<<32 | xor64();

    const int BW = 8;
    vector<State> S[T];

    //  t=0
    for (int p=0; p<N*N; p++)
    {
        State s;
        s.score = 1;
        s.machine[p] = true;
        s.move = Move(-1, p);
        for (auto posv: SPosV[0])
        {
            if (s.machine[posv.pos])
            {
                s.score += posv.v;
                s.money += posv.v;
            }
            else
                s.field[posv.pos] = posv.v;
        }
        for (int p: EPos[0])
            s.field[p] = 0;
        s.hash = calc_hash(s.field, s.machine);

        S[0].push_back(s);
    }

    sort(S[0].begin(), S[0].end());
    if ((int)S[0].size()>BW)
        S[0].resize(BW);

    int dr[] = {1, -1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    for (int turn=1; turn<T; turn++)
    {
        map<unsigned long long, State> MS;

        for (State &s1: S[turn-1])
        {
            int mn = 0;
            for (int p=0; p<N*N; p++)
                if (s1.machine[p])
                    mn++;

            vector<int> from;
            if ((mn+1)*(mn+1)*(mn+1)<=s1.money)
                from.push_back(-1);
            //  最後はマシンを変えるときに移動も可
            if (turn>=T*9/10 ||
                (mn+1)*(mn+1)*(mn+1)>s1.money)
                for (int p=0; p<N*N; p++)
                    if (s1.machine[p])
                        from.push_back(p);
            if ((int)from.size()>8)
            {
                int n = (int)from.size();
                for (int i=0; i<8; i++)
                    swap(from[i], from[xor64()%(n-i)+i]);
                from.resize(8);
            }

            for (int f: from)
            {
                for (int t=0; t<N*N; t++)
                {
                    if (f==t || !s1.machine[t])
                    {
                        if (f>=0)
                            s1.machine[f] = false;
                        s1.machine[t] = true;
                        bool ok = calc_k(s1.machine, t)==mn+(f<0 ? 1 : 0);
                        s1.machine[t] = false;
                        if (f>=0)
                            s1.machine[f] = true;

                        if (ok)
                        {
                            State s2 = s1;
                            s2.prev = &s1;
                            s2.move = Move(f, t);
                            int mn2 = mn;

                            if (f>=0)
                                s2.machine[f] = false;
                            s2.machine[t] = true;
                            if (f<0)
                            {
                                mn2++;
                                s2.money -= mn2*mn2*mn2;
                            }

                            if (s2.field[t]>0)
                            {
                                s2.money += s2.field[t]*mn2;
                                s2.score += s2.field[t]*mn2;
                                s2.field[t] = 0;
                            }
                            for (auto posv: SPosV[turn])
                            {
                                if (s2.machine[posv.pos])
                                {
                                    s2.money += posv.v*mn2;
                                    s2.score += posv.v*mn2;
                                }
                                else
                                    s2.field[posv.pos] = posv.v;
                            }
                            for (int p: EPos[turn])
                                s2.field[p] = 0;

                            //  最後は金額を見る
                            if (turn>=T*9/10)
                                s2.score = s2.money;

                            s2.hash = calc_hash(s2.field, s2.machine);

                            if (MS.count(s2.hash)==0 ||
                                s2.score > MS[s2.hash].score)
                                MS[s2.hash] = s2;
                        }
                    }
                }
            }
        }

        for (auto &s: MS)
            S[turn].push_back(s.second);
        sort(S[turn].begin(), S[turn].end());
        if ((int)S[turn].size()>BW)
            S[turn].resize(BW);
    }

    cerr<<"money: "<<S[T-1][0].money<<endl;
    int mn = 0;
    for (int m: S[T-1][0].machine)
        if (m)
            mn++;
    cerr<<"machine: "<<mn<<endl;

    vector<Move> moves;
    for (State *s = &S[T-1][0]; s!=nullptr; s=s->prev)
        moves.push_back(s->move);
    reverse(moves.begin(), moves.end());

    for (Move move: moves)
        if (move.to>=0)
            if (move.from<0)
                cout<<move.to/N<<" "<<move.to%N<<endl;
            else
                cout<<move.from/N<<" "<<move.from%N<<" "<<move.to/N<<" "<<move.to%N<<endl;
        else
            cout<<-1<<endl;

    cerr<<"time: "<<chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now()-start).count()*1e-6<<endl;
}
