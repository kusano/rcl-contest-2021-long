//  g++ -O2 -o A A.cpp && ./A < tester/input_0.txt > tester/output_0.txt

#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>
#include <cmath>
#include <queue>
using namespace std;

const int N = 16;
const int M = 5000;
const int T = 1000;

struct RC
{
    int r, c;
    RC(int r, int c): r(r), c(c) {}
    bool operator==(const RC &rc) const {return r==rc.r && c==rc.c;}
};

//  Phase 1用
struct Move1
{
    RC rc;
    Move1(RC rc): rc(rc) {}
};

//  Phase 2用
struct Move2
{
    //  購入はrc1.r=-1
    //  パスはrc2.r=-1
    RC rc1, rc2;
    Move2(RC rc1, RC rc2): rc1(rc1), rc2(rc2) {}
};

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

//  Phase 1
//  move[i].rcに置く
//  購入できる資金があるなら買う
//  資金が無いなら最も古いマシンを移動
//  マシンの購入金額を引かず、総獲得資金を返す
long long calc_score1(const vector<Move1> &moves)
{
    int vege[N][N] = {};
    bool machine[N][N] = {};
    long long money = 1;
    long long money_machine = 0;
    int j = 0;
    queue<RC> Q;

    for (int t=0; t<T; t++)
    {
        const RC &rc = moves[t].rc;
        if (machine[rc.r][rc.c])
            return -1;

        int m = (j+1)*(j+1)*(j+1);
        if (m<=money)
        {
            j++;
            money -= m;
            money_machine += m;
        }
        else
        {
            RC f = Q.front();
            Q.pop();
            machine[f.r][f.c] = false;
        }

        Q.push(rc);
        machine[rc.r][rc.c] = true;

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

    return money + money_machine;
}

vector<Move2> convert_moves(const vector<Move1> &moves)
{
    int vege[N][N] = {};
    bool machine[N][N] = {};
    long long money = 1;
    int j = 0;
    queue<RC> Q;
    vector<Move2> result;

    for (int t=0; t<T; t++)
    {
        const RC &rc = moves[t].rc;
        if (machine[rc.r][rc.c])
        {
            result.resize(T, Move2(RC(-1, -1), RC(-1, -1)));
            return result;
        }

        int m = (j+1)*(j+1)*(j+1);
        if (m<=money)
        {
            j++;
            money -= m;
            result.push_back(Move2(RC(-1, -1), rc));
        }
        else
        {
            RC f = Q.front();
            Q.pop();
            machine[f.r][f.c] = false;
            result.push_back(Move2(f, rc));
        }

        Q.push(rc);
        machine[rc.r][rc.c] = true;

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

    return result;
}

long long calc_score2(const vector<Move2> &moves)
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

    vector<Move1> moves;
    for (int i=0; i<T; i++)
        moves.push_back(RC(i/N%N, i%N));

    long long score = 1;
    vector<Move1> best_moves = moves;
    long long best_score = score;
    double time = 0;
    double temp = 1e10;
    int iter;
    for (iter=0; ; iter++)
    {
        if (iter%0x100==0)
        {
            chrono::system_clock::time_point now = chrono::system_clock::now();
            time = chrono::duration_cast<chrono::microseconds>(now-start).count()*1e-6/1.5;
            if (time>=1.)
                break;
            temp = (1-time)*1000;
        }

        int t = xor64()%T;
        Move1 old_move = moves[t];
        moves[t].rc = RC(xor64()%N, xor64()%N);

        long long score2 = calc_score1(moves);
        if (score2<0)
        {
            moves[t] = old_move;
            continue;
        }

        if (score2>best_score)
        {
            best_moves = moves;
            best_score = score2;
        }

        double prob = exp((score2-score)/temp);
        //if (xor64()%0x10000/0x10000<prob)
        if (xor64()%0x10000<prob*0x10000)
            score = score2;
        else
            moves[t] = old_move;
    }

    cerr<<"time: "<<time<<endl;
    cerr<<"iter: "<<iter<<endl;
    cerr<<"best_score: "<<best_score<<endl;

    vector<Move2> moves2 = convert_moves(best_moves);
    vector<Move2> best_moves2 = moves2;
    long long best_score2 = calc_score2(moves2);
    while (true)
    {
        //  最後の新規購入を探す
        int t;
        for (t=T-1; t>=0; t--)
            if (moves2[t].rc2.r!=-1 && moves2[t].rc1.r==-1)
                break;
        if (t<0)
            break;
        //  このマシンをパスに変えていく
        RC rc = moves2[t].rc2;
        moves2[t].rc1 = RC(-1, -1);
        moves2[t].rc2 = RC(-1, -1);
        for (t++; t<T; t++)
            if (moves2[t].rc1==rc)
            {
                rc = moves2[t].rc2;
                moves2[t].rc1 = RC(-1, -1);
                moves2[t].rc2 = RC(-1, -1);
            }
        long long score = calc_score2(moves2);
        //cerr<<score<<endl;
        if (score>best_score2)
        {
            best_score2 = score;
            best_moves2 = moves2;
        }
    }

    cerr<<"best_score2: "<<best_score2<<endl;

    for (Move2 move: best_moves2)
    {
        if (move.rc2.r>=0)
            if (move.rc1.r<0)
                cout<<move.rc2.r<<" "<<move.rc2.c<<endl;
            else
                cout<<move.rc1.r<<" "<<move.rc1.c<<" "<<move.rc2.r<<" "<<move.rc2.c<<endl;
        else
            cout<<-1<<endl;
    }
}
