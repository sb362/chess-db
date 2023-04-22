
#include "chess/pgn.hh"

using namespace cdb;
using namespace chess;

int main(int, char *[]) {
  const std::string pgn = "[Event \"001.Praga\"]"
"[Site \"?\"]"
"[Date \"1929.??.??\"]"
"[Round \"?\"]"
"[White \"Opocensky, Karel\"]"
"[Black \"Flohr, Salo\"]"
"[Result \"0-1\"]"
"[ECO \"D30\"]"
"[Annotator \"Franco Pezzi\"]"
"[PlyCount \"104\"]"
"[EventDate \"1929.??.??\"]"
""
"1. d4 d5 2. Nf3 Nf6 3. c4 e6 4. Nbd2 {Questa mossa, che appare raramente nella"
"pratica dei maestri, fu impiegata da Opocensky anche contro Steiner a Brno nel"
"1928.} 4... c5 5. cxd5 exd5 (5... Qxd5 6. e4 $1 Nxe4 7. Bc4 Qc6 {=} 8. Ne5) 6."
"g3 Nc6 7. Bg2 cxd4 8. O-O d3 {Il tentativo di difendere questo pedone con 8..."
"Bc5 altro non sarebbe che una perdita di tempo. Il vantaggio della mossa del"
"testo consiste nel fatto che l'immediata cattura del pedone chiudera la"
"colonna \"d\" impedendo cosi al Bianco di attaccare con le Torri il debole"
"pedone d5.} 9. exd3 Be7 10. Nb3 O-O 11. Nfd4 Bg4 12. Nxc6 bxc6 13. Qc2 Qb6 14."
"Be3 Qa6 15. Rfc1 Rac8 16. Qd2 $1 Rfe8 17. Nc5 Qb5 ({Il cambio} 17... Bxc5 {"
"non e soddisfacente a causa della risposta} 18. Rxc5 {"
"che minaccia la successiva 19.Ra5.}) 18. a4 {Il Bianco, uscito bene"
"dall'apertura, commette ora la sua prima imprecisione indebolendo l'ala di"
"Donna e specialmente la casa \"b4\". Migliore sarebbe stata 18.Rc2.} 18... Qb8"
"19. Ra3 Qe5 20. h3 {"
"Necessaria. Si minacciava 20..Qh5 con pressione sulle case bianche.} 20... Be6"
"21. d4 Qf5 22. g4 Qg6 23. Nxe6 ({Se} 23. f4 Bxg4 24. hxg4 Nxg4 {"
"con forte attacco.}) 23... fxe6 24. Rb3 Bd6 25. Rb7 h5 $1 26. g5 Ne4 27. Qc2"
"Qf5 {Era minacciata 28.f3. Ora il Nero dispone della risposta 28...Ng3.} 28."
"Qd1 ({Dopo} 28. Rxa7 e5 {il Nero ha la possibilita di impiantare una batteria"
"d'assalto lungo la diagonale b8-h2.}) 28... g6 29. Qf3 Re7 30. Qxf5 exf5 31."
"Rxe7 Bxe7 32. h4 Bb4 33. Rc2 {Una mossa inutile. Il Bianco non avrebbe dovuto"
"temere 33...Bd2 perche dopo 34.Bxd2 Nxd2 avrebbe potuto sfruttare il"
"tatticismo 35.Bxd5+} 33... Kf7 34. f3 Nd6 35. Bf1 Ke6 36. Ba6 Rc7 ({"
"Non andava bene} 36... Re8 {per} 37. Bf4 (37. Rxc6 $2 Kd7 {"
"e i pezzi in presa sono due.}) 37... Kd7 38. Bxd6 Kxd6 39. Rxc6+) 37. Kf2 Nc4"
"$1 {Un sacrificio di pedone con molti obiettivi: togliere all'avversario la"
"coppia degli Alfieri, liberare la casa d5 per il proprio Re e permettere alla"
"Torre nera di conquistare la colonna \"b\" utilissima per attaccare i deboli"
"pedoni bianchi.} 38. Bxc4 {"
"Sarebbe stato meglio rifiutare il sacrificio continuando con 38.Bf4 Bd6 39.Bc1."
"} 38... dxc4 39. Bf4 Rb7 40. Rxc4 Kd5 41. b3 Bf8 $1 ({"
"Sarebbe stato un grosso errore giocare} 41... Bd6 42. Bxd6 Rxb3 $2 {per} 43."
"Rb4 {e il Bianco avrebbe vinto facilmente.}) 42. Rc3 Rb4 43. Be3 (43. Be5 c5"
"44. f4 c4 $1 45. bxc4+ Rxc4 46. Rb3 Bb4 {seguita da a7-a5 e Rc4-c2-a2.}) 43..."
"f4 $1 44. Bxf4 Rxd4 45. Kg3 {=} 45... Bb4 $1 {Un vero centrocampista!} 46. Rc4"
"{Obbligata.} (46. Rc2 Be1+) (46. Rc1 Rxf4 47. Kxf4 Bd2+) (46. Re3 Rxf4 47. Kxf4"
"Bd6+) 46... Be1+ 47. Kg2 Rxc4 48. bxc4+ Kxc4 49. Bb8 Bxh4 50. f4 a6 51. Be5 Kd5"
"52. f5 $2 {Probabile cappella \"zeitnottiana\".} 52... Kxe5 {Il Bianco abbandona."
"} 0-1";

  auto r = parse_tags(pgn, [] (auto n, auto v) {
    std::cout << n << " " << v << '\n';
  });
  std::cout << '\n';

  if (r.ec) {
    std::cerr << " err: " << r.ec.message() << '\n';
    std::cerr << " msg: " << r.msg << '\n';
    std::cerr << " pos: " << r.bytes_read << '\n';
    return -1;
  }

  r = parse_movetext(pgn.substr(r.bytes_read), [] (const ParseStep &step) {
    std::cout << step.bytes_read << ' '
              << step.san << '\t'
              << step.move << '\t'
              << step.comment << '\t'
              << step.prev.to_fen(step.move_no % 2 == 0) << '\t'
              << step.next.to_fen(step.move_no % 2) << '\n';
  });
  if (r.ec) {
    std::cerr << " err: " << r.ec.message() << '\n';
    std::cerr << " msg: " << r.msg << '\n';
    std::cerr << " pos: " << r.bytes_read << '\n';
    return -1;
  }

  return 0;
}
