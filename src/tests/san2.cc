#include "chess/notation.hh"
#include "chess/movegen.hh"

#include <format>
#include <iostream>
#include <sstream>
#include <string>

const std::string test = "1. e4 c5 2. e5 d5 3. exd6 Nf6 4. d7+ Nbxd7 5. Bb5 a5 6. b4 axb4 "
                         "7. c4 bxc3 8. Nxc3 e5 9. d4 exd4 10. Qe2+ Qe7 11. Qxe7+ Kxe7 "
                         "12. Bg5 Nb6 13. Nd5+ Nxd5 14. Nf3 Bf5 15. O-O-O Rxa2 16. Bd3 Bxd3 "
                         "17. Rxd3 Ke8 18. Re1+ Ne3 19. fxe3 Bd6 20. exd4+ Kf8 21. dxc5 Ra1+ "
                         "22. Kb2 Rxe1 23. Nxe1 b5 24. cxd6 h6 25. Bxf6 gxf6 26. d7 Kg7 "
                         "27. d8=N Rxd8 28. Rxd8 b4 29. Nf3 b3 30. Kc3 b2 31. h4 b1=Q "
                         "32. Rg8+ Kxg8 33. Ng5 fxg5 34. hxg5 hxg5 35. g4 Qg1 36. Kd3 f5 "
                         "37. gxf5 g4 38. f6 g3 39. f7+ Kg7 40. f8=B+ Kg6 41. Bh6 g2 "
                         "42. Be3 Qe1 43. Kc2 g1=Q 44. Kd3 Qgg3 45. Kd4 Qe5+ 46. Kd3 Q5xe3+ 47. Kc2 Q3c3#";

using namespace cdb::chess;

int main(int, char *[]) {
  std::stringstream ss {test};

  ss >> std::skipws;

  std::string san;
  bool black = false;
  Position pos = startpos;

  while (ss >> san) {
    // skip move numbers
    if (std::isdigit(san[0])) continue;

    std::cout << san << std::endl;

    const auto move = parse_san(san, pos, black);
    assert(move);

    const auto new_san = to_san(*move, pos, black);
    if (new_san != san) {
      std::cerr << "In position:\n  " << pos.to_fen(black) << '\n';
      std::cerr << std::format("    {:x}\n    {:x}\n    {:x}\n    {:x}\n", pos.x, pos.y, pos.z, pos.white);
      std::cerr << "Expected " << san << ", got " << new_san << '\n';
      return -1;
    }

    black ^= 1;
    pos = make_move(pos, *move);
  }

  return 0;
}
