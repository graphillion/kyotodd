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

  if (bddinit(1ULL << 20, 0)) {
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
  std::printf("(grid set up; cycle ZDD not yet implemented)\n");

  return 0;
}
