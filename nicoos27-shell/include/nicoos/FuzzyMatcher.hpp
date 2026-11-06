#pragma once

#include <QString>
#include <QVector>
#include "nicoos/AppIndexer.hpp"

namespace nicoos {

struct MatchResult {
    DesktopApp app;
    int score{};
};

// Semplice matcher fuzzy basato su sottosequenze (nello spirito del
// punteggio di fzf/rofi): premia le corrispondenze di caratteri consecutivi,
// quelle ai confini di parola e quelle vicine all'inizio della stringa.
// Volutamente senza dipendenze e O(n*m) per candidato, che è più che
// sufficientemente veloce per qualche centinaio di voci desktop digitate in
// modo interattivo.
class FuzzyMatcher {
public:
    static QVector<MatchResult> match(const QString& query, const QVector<DesktopApp>& apps);

private:
    static int scoreSingle(const QString& query, const QString& target);
};

} // namespace nicoos
