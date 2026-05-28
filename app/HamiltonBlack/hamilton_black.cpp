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

#include "bdd.h"

using namespace kyotodd;

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

  // BDD パッケージ初期化。実用的な節点数上限を設定。
  if (bddinit(1ULL << 20, 0)) {
    std::fprintf(stderr, "bddinit failed\n");
    return 1;
  }

  std::printf("HamiltonBlack: %d x %d grid\n", H, W);
  std::printf("(skeleton only — solver not implemented yet)\n");

  return 0;
}
