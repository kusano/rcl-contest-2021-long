//  g++ -O2 -o A A.cpp && python3 test.py

#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>
#include <cmath>
#include <queue>
#include <cstring>
#include <algorithm>
#include <unordered_map>
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
    int machine_number = 0;
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

unsigned long long Hash[N*N];
unsigned char Neighbor[N*N][4]; // Neighbor[i][x]==i は無効値
int MachinePrice[N*N];          //  i番目のマシンを買うときの値段
int MachineTotal[N*N+1];        //  i個のマシンを買うのに使った金額

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

    int n = 1;
    for (int t: Neighbor[p])
        if (t!=p &&
            machine[t])
            n += calc_k_sub(machine, t, hist);
    return n;
}

long long calc_hash(bool M[N*N])
{
    long long h = 0;
    for (int p=0; p<N*N; p++)
        if (M[p])
            h ^= Hash[p];
    return h;
}

long long calc_score(int turn, State &s)
{
    long long score;
    //  最後は金を見る
    if (turn>=T*9/10)
        score = s.money;
    else
        score = s.money + MachineTotal[s.machine_number];

    score = score<<16 | (long long)(s.hash&0xffff);
    return score;
}

void init()
{
    for (int p=0; p<N*N; p++)
        Hash[p] = (unsigned long long)xor64()<<32 | xor64();

    int dr[] = {1, -1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    for (int p=0; p<N*N; p++)
    {
        int r = p/N;
        int c = p%N;
        for (int d=0; d<4; d++)
        {
            int tr = r+dr[d];
            int tc = c+dc[d];
            if (0<=tr && tr<N &&
                0<=tc && tc<N)
                Neighbor[p][d] = (unsigned char)(tr*N+tc);
            else
                Neighbor[p][d] = p;
        }
    }

    for (int i=0; i<256; i++)
        MachinePrice[i] = (i+1)*(i+1)*(i+1);
    MachineTotal[0] = 0;
    for (int i=1; i<=256; i++)
        MachineTotal[i] = MachineTotal[i-1]+MachinePrice[i-1];
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

    init();

    const int BW = 16;
    vector<State> S[T];

    //  t=0
    for (int p=0; p<N*N; p++)
    {
        State s;
        s.score = 1;
        s.machine[p] = true;
        s.move = Move(-1, p);
        s.machine_number = 1;
        for (auto posv: SPosV[0])
        {
            if (s.machine[posv.pos])
                s.money += posv.v;
            else
                s.field[posv.pos] = posv.v;
        }
        for (int p: EPos[0])
            s.field[p] = 0;
        s.hash = calc_hash(s.machine);
        s.score = calc_score(0, s);

        S[0].push_back(s);
    }

    sort(S[0].begin(), S[0].end());
    if ((int)S[0].size()>BW)
        S[0].resize(BW);

    vector<State> Stemp;
    unordered_map<unsigned long long, State> MS;

    for (int turn=1; turn<T; turn++)
    {
        MS.clear();

        for (State &s1: S[turn-1])
        {
            vector<int> from;
            //  最後はマシンを買えるときに移動も可
            if (turn>=T*9/10 ||
                MachinePrice[s1.machine_number]>s1.money)
            {
                for (int p=0; p<N*N; p++)
                    if (s1.machine[p])
                    {
                        //  移動元は除いても連結にする
                        bool ok;
                        if (s1.machine_number==1)
                            ok = true;
                        else
                        {
                            s1.machine[p] = false;
                            for (int t: Neighbor[p])
                                if (t!=p &&
                                    s1.machine[t])
                                {
                                    ok = calc_k(s1.machine, t)==s1.machine_number-1;
                                    break;
                                }
                            s1.machine[p] = true;
                        }

                        if (ok)
                            from.push_back(p);
                    }
            }
            if ((int)from.size()>3)
            {
                int n = (int)from.size();
                for (int i=0; i<3; i++)
                    swap(from[i], from[xor64()%(n-i)+i]);
                from.resize(3);
            }
            if (MachinePrice[s1.machine_number]<=s1.money)
                from.push_back(-1);

            for (int f: from)
            {
                for (int t=0; t<N*N; t++)
                {
                    if (f==t || !s1.machine[t])
                    {
                        //  移動元以外のいずれかのマシンと隣接していれば置ける
                        bool ok;
                        if (s1.machine_number+(f<0 ? 1 : 0)==1)
                            ok = true;
                        else if (f==t)
                            ok = true;
                        else
                        {
                            ok = false;
                            for (int tt: Neighbor[t])
                                if (tt!=t &&
                                    tt!=f &&
                                    s1.machine[tt])
                                {
                                    ok = true;
                                    break;
                                }
                        }

                        if (ok)
                        {
                            State s2 = s1;
                            s2.prev = &s1;
                            s2.move = Move(f, t);

                            if (f>=0)
                            {
                                s2.machine[f] = false;
                                s2.hash ^= Hash[f];
                            }
                            s2.machine[t] = true;
                            s2.hash ^= Hash[t];

                            if (f<0)
                            {
                                s2.money -= MachinePrice[s2.machine_number];
                                s2.machine_number++;
                            }

                            if (s2.field[t]>0)
                            {
                                s2.money += s2.field[t]*s2.machine_number;
                                s2.field[t] = 0;
                            }
                            for (auto posv: SPosV[turn])
                            {
                                if (s2.machine[posv.pos])
                                    s2.money += posv.v*s2.machine_number;
                                else
                                    s2.field[posv.pos] = posv.v;
                            }
                            for (int p: EPos[turn])
                                s2.field[p] = 0;

                            s2.score = calc_score(turn, s2);

                            if (MS.count(s2.hash)==0 ||
                                s2.score > MS[s2.hash].score)
                                MS[s2.hash] = s2;
                        }
                    }
                }
            }
        }

        Stemp.clear();
        for (auto &s: MS)
            Stemp.push_back(s.second);
        sort(Stemp.begin(), Stemp.end());
        if ((int)Stemp.size()>BW)
            Stemp.resize(BW);
        for (State &s: Stemp)
            S[turn].push_back(s);
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
