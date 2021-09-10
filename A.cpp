//  g++ -O2 -o A A.cpp && for i in $(seq 9); do echo ${i}; ./A < tester/input_${i}.txt > tester/output_${i}.txt; done

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

struct RC
{
    int r, c;
    RC(): r(-1), c(-1) {}
    RC(int r, int c): r(r), c(c) {}
    bool operator==(const RC &rc) const {return r==rc.r && c==rc.c;}
};

struct Move
{
    //  購入はrc1.r=-1
    //  パスはrc2.r=-1
    RC rc1, rc2;
    Move(): rc1(-1, -1), rc2(-1, -1) {}
    Move(RC rc1, RC rc2): rc1(rc1), rc2(rc2) {}
};

struct State
{
    short F[N][N] = {};
    bool M[N][N] = {};
    unsigned long long hash = 0;
    long long score = 0;
    long long money = 0;
    State *prev = nullptr;
    Move move;
};

bool operator<(const State &s1, const State &s2)
{
    return s1.score>s2.score;
}

unsigned long long Hash[N][N][3];

vector<RC> SRC[T];
vector<int> SV[T];
vector<RC> ERC[T];

int xor64(void) {
    static uint64_t x = 88172645463325252ULL;
    x ^= x<<13;
    x ^= x>> 7;
    x ^= x<<17;
    return int(x&0x7fffffff);
}

//  隣接しているマシン数を返す
int calc_k_sub(bool machine[N][N], int r, int c, vector<RC> *hist);

int calc_k(bool machine[N][N], int r, int c)
{
    assert(machine[r][c]);

    static vector<RC> hist;
    int n = calc_k_sub(machine, r, c, &hist);

    while (!hist.empty())
    {
        machine[hist.back().r][hist.back().c] = true;
        hist.pop_back();
    }

    return n;
}

int calc_k_sub(bool machine[N][N], int r, int c, vector<RC> *hist)
{
    assert(machine[r][c]);

    machine[r][c] = false;
    hist->push_back(RC(r, c));

    static int dr[] = {-1, 1, 0, 0};
    static int dc[] = {0, 0, -1, 1};
    int n = 1;
    for (int d=0; d<4; d++)
    {
        int tr = r + dr[d];
        int tc = c + dc[d];
        if (0<=tr && tr<N &&
            0<=tc && tc<N &&
            machine[tr][tc])
            n += calc_k_sub(machine, tr, tc, hist);
    }
    return n;
}

long long calc_score(const vector<Move> &moves)
{
    int vege[N][N] = {};
    bool machine[N][N] = {};
    long long money = 1;
    int j = 0;

    for (int t=0; t<T; t++)
    {
        const RC &rc1 = moves[t].rc1;
        const RC &rc2 = moves[t].rc2;

        if (rc2.r>=0)
        {
            if (rc1.r<0)
            {
                //  購入
                int m = (j+1)*(j+1)*(j+1);
                j++;
                if (money<m)
                    return -1;
                money -= m;
            }
            else
            {
                //  移動
                if (!machine[rc1.r][rc1.c])
                    return -1;
                machine[rc1.r][rc1.c] = false;
            }

            if (machine[rc2.r][rc2.c])
                return -1;
            machine[rc2.r][rc2.c] = true;
        }

        for (int i=0; i<(int)SRC[t].size(); i++)
            vege[SRC[t][i].r][SRC[t][i].c] = SV[t][i];

        for (int r=0; r<N; r++)
            for (int c=0; c<N; c++)
                if (vege[r][c]>0 && machine[r][c])
                {
                    int v = vege[r][c];
                    vege[r][c] = 0;
                    int k = calc_k(machine, r, c);
                    money += (long long)v*k;
                }

        for (RC rc: ERC[t])
            vege[rc.r][rc.c] = 0;
    }

    return money;
}

long long calc_hash(short F[N][N], bool M[N][N])
{
    long long h = 0;
    for (int r=0; r<N; r++)
        for (int c=0; c<N; c++)
            h ^= Hash[r][c][(F[r][c]>0?2:0)+(M[r][c]?1:0)];
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
        SRC[S].push_back(RC(R, C));
        SV[S].push_back(V);
        ERC[E].push_back(RC(R, C));
    }

    chrono::system_clock::time_point start = chrono::system_clock::now();

    for (int i=0; i<N; i++)
        for (int j=0; j<N; j++)
            for (int k=0; k<3; k++)
                Hash[i][j][k] = (unsigned long long)xor64()<<32 | xor64();

    const int BW = 8;
    vector<State> S[T];

    //  t=0
    for (int r=0; r<N; r++)
        for (int c=0; c<N; c++)
        {
            State s;
            s.score = 1;
            s.M[r][c] = true;
            s.move = Move(RC(-1, -1), RC(r, c));
            for (int i=0; i<(int)SRC[0].size(); i++)
            {
                const RC &rc = SRC[0][i];
                if (s.M[rc.r][rc.c])
                {
                    s.score += SV[0][i];
                    s.money += SV[0][i];
                }
                else
                    s.F[rc.r][rc.c] += SV[0][i];
            }
            for (RC rc: ERC[0])
                s.F[rc.r][rc.c] = 0;
            s.hash = calc_hash(s.F, s.M);

            S[0].push_back(s);
        }

    sort(S[0].begin(), S[0].end());
    if ((int)S[0].size()>BW)
        S[0].resize(BW);

    int dr[] = {1, -1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    for (int t=0; t<T-1; t++)
    {
        map<unsigned long long, State> MS;

        for (State &s1: S[t])
        {
            int mn = 0;
            for (int r=0; r<N; r++)
                for (int c=0; c<N; c++)
                    if (s1.M[r][c])
                        mn++;

            vector<RC> from;
            if ((mn+1)*(mn+1)*(mn+1)<=s1.money)
                from.push_back(RC(-1, -1));
            else
                for (int r=0; r<N; r++)
                    for (int c=0; c<N; c++)
                        if (s1.M[r][c])
                            from.push_back(RC(r, c));
            if ((int)from.size()>8)
            {
                int n = (int)from.size();
                for (int i=0; i<8; i++)
                    swap(from[i], from[xor64()%(n-i)+i]);
                from.resize(8);
            }

            for (RC f: from)
            {
                for (int tr=0; tr<N; tr++)
                    for (int tc=0; tc<N; tc++)
                    {
                        RC trc(tr, tc);
                        if (f==trc || !s1.M[tr][tc])
                        {
                            if (f.r>=0)
                                s1.M[f.r][f.c] = false;
                            s1.M[tr][tc] = true;
                            bool ok = calc_k(s1.M, tr, tc)==mn+(f.r<0 ? 1 : 0);
                            s1.M[tr][tc] = false;
                            if (f.r>=0)
                                s1.M[f.r][f.c] = true;

                            if (ok)
                            {
                                State s2;
                                memcpy(s2.F, s1.F, sizeof s2.F);
                                memcpy(s2.M, s1.M, sizeof s2.M);
                                s2.score = s1.score;
                                s2.money = s1.money;
                                s2.prev = &s1;
                                s2.move = Move(f, trc);

                                if (f.r>=0)
                                    s2.M[f.r][f.c] = false;
                                s2.M[tr][tc] = true;
                                if (f.r<0)
                                {
                                    mn++;
                                    s2.money -= mn*mn*mn;
                                }

                                if (s2.F[tr][tc]>0)
                                {
                                    s2.money += s2.F[tr][tc]*mn;
                                    s2.score += s2.F[tr][tc]*mn;
                                    s2.F[tr][tc] = 0;
                                }
                                for (int i=0; i<(int)SRC[t+1].size(); i++)
                                {
                                    RC &rc = SRC[t+1][i];
                                    if (s2.M[rc.r][rc.c])
                                    {
                                        s2.money += SV[t+1][i]*mn;
                                        s2.score += SV[t+1][i]*mn;
                                    }
                                    else
                                        s2.F[rc.r][rc.c] = SV[t+1][i];
                                }
                                for (RC rc: ERC[t+1])
                                    s2.F[rc.r][rc.c] = 0;

                                s2.hash = calc_hash(s2.F, s2.M);

                                if (MS.count(s2.hash)==0 ||
                                    s2.score > MS[s2.hash].score)
                                    MS[s2.hash] = s2;
                            }
                        }
                    }
            }
        }

        for (auto &s: MS)
            S[t+1].push_back(s.second);
        sort(S[t+1].begin(), S[t+1].end());
        if ((int)S[t+1].size()>BW)
            S[t+1].resize(BW);

        //cerr<<t+1<<" "<<S[t+1][0].score<<" "<<S[t+1][0].money<<endl;
    }

    cerr<<"score: "<<S[T-1][0].score<<endl;
    cerr<<"money: "<<S[T-1][0].money<<endl;
    int mn = 0;
    for (int r=0; r<N; r++)
        for (int c=0; c<N; c++)
            if (S[T-1][0].M[r][c])
                mn++;
    cerr<<"machine: "<<mn<<endl;

    vector<Move> moves;
    for (State *s = &S[T-1][0]; s!=nullptr; s=s->prev)
        moves.push_back(s->move);
    reverse(moves.begin(), moves.end());

    vector<Move> best_moves = moves;
    long long best_score = calc_score(moves);
    while (true)
    {
        //  最後の新規購入を探す
        int t;
        for (t=T-1; t>=0; t--)
            if (moves[t].rc2.r!=-1 && moves[t].rc1.r==-1)
                break;
        if (t<0)
            break;
        //  このマシンをパスに変えていく
        RC rc = moves[t].rc2;
        moves[t].rc1 = RC(-1, -1);
        moves[t].rc2 = RC(-1, -1);
        for (t++; t<T; t++)
            if (moves[t].rc1==rc)
            {
                rc = moves[t].rc2;
                moves[t].rc1 = RC(-1, -1);
                moves[t].rc2 = RC(-1, -1);
            }
        long long score = calc_score(moves);
        //cerr<<score<<endl;
        if (score>best_score)
        {
            best_score = score;
            best_moves = moves;
        }
    }

    cerr<<"score: "<<best_score<<endl;

    for (Move move: best_moves)
    {
        if (move.rc2.r>=0)
            if (move.rc1.r<0)
                cout<<move.rc2.r<<" "<<move.rc2.c<<endl;
            else
                cout<<move.rc1.r<<" "<<move.rc1.c<<" "<<move.rc2.r<<" "<<move.rc2.c<<endl;
        else
            cout<<-1<<endl;
    }

    cerr<<"time: "<<chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now()-start).count()*1e-6<<endl;
}
