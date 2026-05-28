// HamiltonBlack — H×W グリッドにおける、隣接しない最大独立黒マス配置
// (黒マスを除いた領域がハミルトン閉路 = 単純閉路 1 本で覆われる条件付き)
//
// 解法:
//   1. グリッドの辺変数上で、単純閉路の集合を Frontier 法により ZDD F_E
//      として構築する。
//   2. F_E を辺→頂点に変換し、各閉路が訪れる頂点集合の ZDD F_V を得る。
//   3. F_V の各集合 S を全頂点集合 V との差 V\S に置き換え、黒マス候補
//      の集合の ZDD F_B を得る。
//   4. 「隣接する 2 マスを同時に黒にしない」独立集合族 I_V との intersection
//      F_B & I_V を取り、それを最大化する。
//   5. 最大黒マス数と、その達成配置 1 つを ASCII 図で出力する。
//
// 引数: ./hamilton_black <H> <W>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bdd.h"

using namespace kyotodd;

// グリッドの頂点と辺、および対応する bddvar を保持する構造体。
//
// 変数割当ルール (ZDD の自然な top-down 構築のため):
//   - 頂点変数を先に確保 -> level 1..H*W に来る (低 level)。
//   - 辺変数を後で確保。さらに、Frontier 法で「最初に処理する辺 (edge index 0)」が
//     ZDD の最上位に来るよう、edge_var[i] には逆順に bddvar を割り当てる。
//     具体的には、bddnewvar を m 回呼んで得たリスト raw_edge_vars (level H*W+1, H*W+2, ..., H*W+m)
//     を逆向きに参照して edge_var[i] = raw_edge_vars[m-1-i] とする。
//     これにより edge_var[0] は H*W+m (最高 level)、edge_var[m-1] は H*W+1。
struct Grid {
  int H, W;
  int n_vertices;          // = H*W
  int n_edges;             // = H*(W-1) + (H-1)*W

  // 頂点インデックス (0..H*W-1) → bddvar
  std::vector<bddvar> vertex_var;

  // 辺インデックス (0..m-1) → bddvar (frontier 用; edge 0 が最高 level)
  std::vector<bddvar> edge_var;

  // 辺 i の両端頂点インデックス (u <= v とは限らない)
  std::vector<std::pair<int,int>> edge_endpoints;

  // 各頂点 v に対し、v に接続する辺の index リスト
  std::vector<std::vector<int>> incident_edges;

  // Frontier 法で使う first_edge[v] / last_edge[v]: v が初めて出現する辺 index と、
  // 最後に出現する辺 index。 v に接続する辺が 0 個の場合は -1 (このコードでは通常起こらない)
  std::vector<int> first_edge;
  std::vector<int> last_edge;

  // セル (i,j) -> 頂点インデックス
  int cell_to_vertex(int i, int j) const { return i * W + j; }
};

// Grid を構築し、bddvar を割り当てる。
// 辺の順序: 行 i = 0..H-1, 列 j = 0..W-1 の順で、各セル (i,j) から
// 右隣 (j+1<W) → 下隣 (i+1<H) の順に辺を発行。
// Frontier の幅はおおむね W+1 程度に収まる。
static Grid build_grid(int H, int W) {
  Grid g;
  g.H = H;
  g.W = W;
  g.n_vertices = H * W;
  g.n_edges = H * (W - 1) + (H - 1) * W;

  // 頂点変数を H*W 個確保 (level 1..H*W)
  g.vertex_var.resize(g.n_vertices);
  for (int v = 0; v < g.n_vertices; ++v) {
    g.vertex_var[v] = bddnewvar();
  }

  // 辺リストを構築 (順序が ZDD frontier 法の処理順となる)
  g.edge_endpoints.reserve(g.n_edges);
  for (int i = 0; i < H; ++i) {
    for (int j = 0; j < W; ++j) {
      int v = g.cell_to_vertex(i, j);
      if (j + 1 < W) {
        g.edge_endpoints.emplace_back(v, g.cell_to_vertex(i, j + 1));
      }
      if (i + 1 < H) {
        g.edge_endpoints.emplace_back(v, g.cell_to_vertex(i + 1, j));
      }
    }
  }

  // 辺変数を割り当て。bddnewvar の戻り値は順に level H*W+1, H*W+2, ... となる。
  // edge_var[0] を最上位 (level H*W+m) にしたいので、逆順に割り当てる。
  std::vector<bddvar> raw_edge_vars(g.n_edges);
  for (int k = 0; k < g.n_edges; ++k) {
    raw_edge_vars[k] = bddnewvar();
  }
  g.edge_var.resize(g.n_edges);
  for (int i = 0; i < g.n_edges; ++i) {
    g.edge_var[i] = raw_edge_vars[g.n_edges - 1 - i];
  }

  // incident_edges, first_edge, last_edge を計算
  g.incident_edges.assign(g.n_vertices, {});
  g.first_edge.assign(g.n_vertices, -1);
  g.last_edge.assign(g.n_vertices, -1);
  for (int i = 0; i < g.n_edges; ++i) {
    int u = g.edge_endpoints[i].first;
    int v = g.edge_endpoints[i].second;
    g.incident_edges[u].push_back(i);
    g.incident_edges[v].push_back(i);
    if (g.first_edge[u] < 0) g.first_edge[u] = i;
    if (g.first_edge[v] < 0) g.first_edge[v] = i;
    g.last_edge[u] = i;
    g.last_edge[v] = i;
  }

  return g;
}

// -----------------------------------------------------------------------------
// Frontier 法による単純閉路 ZDD の構築
// -----------------------------------------------------------------------------
//
// 状態 (State): 各頂点 v に対して mate[v] を保持。エンコーディング:
//   NOT_PRESENT (= -1): フロンティアに居ない (未到来 or 既に去った、deg 0 相当)
//   SATURATED   (= -2): フロンティアに居て deg 2。これ以上接続辺を取れない。
//   それ以外 (>= 0):  フロンティアに居て deg 1。path のもう一方の端点 index。
//
// さらに cycle_closed (bool): 既に閉路を 1 本作ったか。閉路は本問題では 1 本のみ許可。
//
// 単純閉路条件:
//   - すべての頂点について最終的に deg ∈ {0, 2}。
//   - 閉路が「ちょうど 1 つ」存在 (cycle_closed が真で、その後新規辺は取らない)。
//   - 連結性は「閉路 1 本」=連結 1 成分なので、cycle_closed の制約のみで担保される
//     (cycle_closed=true 状態で別の新規パスを開始すると、それは 2 本目の閉路を要求
//      してしまい cycle_closed=false 経由でしか取れない=矛盾、したがって弾かれる)。

static constexpr int NOT_PRESENT = -1;
static constexpr int SATURATED   = -2;

struct State {
  std::vector<int> mate;
  bool cycle_closed;

  bool operator==(const State& other) const {
    return cycle_closed == other.cycle_closed && mate == other.mate;
  }
};

struct StateHash {
  size_t operator()(const State& s) const {
    size_t h = static_cast<size_t>(s.cycle_closed ? 1 : 0);
    for (int x : s.mate) {
      h ^= static_cast<size_t>(x + 3);
      h *= 1099511628211ULL;
    }
    return h;
  }
};

static inline int degree_of(const State& s, int w) {
  int m = s.mate[w];
  if (m == NOT_PRESENT) return 0;
  if (m == SATURATED)   return 2;
  return 1;
}

// edge (u,v) を「選択」したときの状態遷移。状態 s を破壊的に変更する。
// 不可能な遷移なら false を返し、s の内容は未定義。
static bool apply_select(State& s, int u, int v) {
  if (s.cycle_closed) return false;            // 既に閉路があるので新規辺は取れない
  int du = degree_of(s, u), dv = degree_of(s, v);
  if (du == 2 || dv == 2) return false;        // 飽和頂点には辺を足せない

  if (du == 0 && dv == 0) {
    // 新規 path u-v
    s.mate[u] = v;
    s.mate[v] = u;
  } else if (du == 0 && dv == 1) {
    int mv = s.mate[v];
    s.mate[u]  = mv;
    s.mate[mv] = u;
    s.mate[v]  = SATURATED;
  } else if (du == 1 && dv == 0) {
    int mu = s.mate[u];
    s.mate[v]  = mu;
    s.mate[mu] = v;
    s.mate[u]  = SATURATED;
  } else {
    // du == 1 && dv == 1
    int mu = s.mate[u], mv = s.mate[v];
    if (mu == v) {
      // u と v は同じ path の両端: 閉路化
      s.cycle_closed = true;
      s.mate[u] = SATURATED;
      s.mate[v] = SATURATED;
    } else {
      // 異なる path 同士をマージ
      s.mate[mu] = mv;
      s.mate[mv] = mu;
      s.mate[u]  = SATURATED;
      s.mate[v]  = SATURATED;
    }
  }
  return true;
}

// edge_idx の処理直後、頂点 w がフロンティアから退去する場合の検査と片付け。
// deg 1 で退去なら dangling path なので invalid (false)。
// deg 0 はそのまま、deg 2 は SATURATED→NOT_PRESENT へ戻す (同等状態の正規化)。
static bool apply_leave_frontier(State& s, int w, int edge_idx, const Grid& g) {
  if (g.last_edge[w] != edge_idx) return true;
  int m = s.mate[w];
  if (m == NOT_PRESENT) return true;
  if (m == SATURATED) {
    s.mate[w] = NOT_PRESENT;
    return true;
  }
  return false;  // deg 1 で去る = 不正
}

struct CycleBuilder {
  const Grid& g;
  std::vector<std::unordered_map<State, ZDD, StateHash>> memos;

  explicit CycleBuilder(const Grid& g_) : g(g_), memos(g_.n_edges + 1) {}

  ZDD build(int i, const State& s) {
    if (i == g.n_edges) {
      return s.cycle_closed ? ZDD::Single : ZDD::Empty;
    }
    auto it = memos[i].find(s);
    if (it != memos[i].end()) return it->second;

    int u = g.edge_endpoints[i].first;
    int v = g.edge_endpoints[i].second;

    // no-select 遷移
    State s_no = s;
    bool ok_no = apply_leave_frontier(s_no, u, i, g)
              && apply_leave_frontier(s_no, v, i, g);
    ZDD f_no = ok_no ? build(i + 1, s_no) : ZDD::Empty;

    // select 遷移
    State s_yes = s;
    bool ok_yes = apply_select(s_yes, u, v);
    if (ok_yes) {
      ok_yes = apply_leave_frontier(s_yes, u, i, g)
            && apply_leave_frontier(s_yes, v, i, g);
    }
    ZDD f_yes = ok_yes ? build(i + 1, s_yes) : ZDD::Empty;
    ZDD f_yes_with_edge = f_yes.Change(g.edge_var[i]);

    ZDD result = f_no + f_yes_with_edge;
    memos[i][s] = result;
    return result;
  }
};

static ZDD build_cycle_zdd(const Grid& g) {
  CycleBuilder builder(g);
  State init;
  init.mate.assign(g.n_vertices, NOT_PRESENT);
  init.cycle_closed = false;
  return builder.build(0, init);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::fprintf(stderr, "Usage: %s <H> <W>\n", argv[0]);
    return 1;
  }
  int H = std::atoi(argv[1]);
  int W = std::atoi(argv[2]);
  if (H <= 0 || W <= 0) {
    std::fprintf(stderr, "H and W must be positive integers.\n");
    return 1;
  }

  // node_count: 初期割当, node_max: デフォルト (UINT64_MAX)。
  if (bddinit(1024)) {
    std::fprintf(stderr, "bddinit failed\n");
    return 1;
  }

  Grid g = build_grid(H, W);

  std::printf("HamiltonBlack: %d x %d grid\n", H, W);
  std::printf("  vertices: %d, edges: %d\n", g.n_vertices, g.n_edges);
  std::printf("  vertex_var[0..%d] -> bddvar [%u..%u]\n",
              g.n_vertices - 1,
              g.vertex_var.front(),
              g.vertex_var.back());
  std::printf("  edge_var[0]=%u (top), edge_var[%d]=%u (bottom-among-edges)\n",
              g.edge_var.front(),
              g.n_edges - 1,
              g.edge_var.back());

  std::printf("Building simple-cycle ZDD over edge variables ...\n");
  ZDD F_E = build_cycle_zdd(g);
  std::printf("  F_E.raw_size() = %llu\n",
              static_cast<unsigned long long>(F_E.raw_size()));
  std::printf("  number of simple cycles = %s\n",
              F_E.exact_count().to_string().c_str());

  return 0;
}
