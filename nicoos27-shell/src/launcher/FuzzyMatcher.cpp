#include "nicoos/FuzzyMatcher.hpp"

#include <algorithm>

namespace nicoos {

int FuzzyMatcher::scoreSingle(const QString& query, const QString& target) {
    if (query.isEmpty()) return 1;

    const QString q = query.toLower();
    const QString t = target.toLower();

    int qi = 0;
    int score = 0;
    int consecutiveBonus = 0;
    bool prevMatched = false;

    for (int ti = 0; ti < t.size() && qi < q.size(); ++ti) {
        if (t[ti] == q[qi]) {
            int charScore = 10;

            // Bonus per una corrispondenza subito dopo un separatore o
            // all'inizio assoluto: premia le corrispondenze allineate ai
            // confini di parola, ad esempio "vc" con "Visual Studio Code".
            if (ti == 0 || t[ti - 1] == ' ' || t[ti - 1] == '-' || t[ti - 1] == '_') {
                charScore += 15;
            }

            if (prevMatched) {
                consecutiveBonus += 8;
                charScore += consecutiveBonus;
            } else {
                consecutiveBonus = 0;
            }

            score += charScore;
            prevMatched = true;
            ++qi;
        } else {
            prevMatched = false;
        }
    }

    if (qi < q.size()) {
        // Non tutti i caratteri della query sono stati trovati come
        // sottosequenza ordinata.
        return -1;
    }

    // Leggera preferenza per i bersagli complessivamente più corti, a parità
    // di tutto il resto (favorisce le corrispondenze esatte o quasi rispetto a
    // stringhe lunghe che contengono la sottosequenza per caso).
    score -= t.size() / 4;

    return score;
}

QVector<MatchResult> FuzzyMatcher::match(const QString& query, const QVector<DesktopApp>& apps) {
    QVector<MatchResult> results;
    results.reserve(apps.size());

    for (const auto& app : apps) {
        int nameScore = scoreSingle(query, app.name);
        int commentScore = app.comment.isEmpty() ? -1 : scoreSingle(query, app.comment) - 30;

        int best = std::max(nameScore, commentScore);
        if (query.isEmpty()) {
            best = 0; // mostra l'elenco completo, stabile alfabeticamente
        }
        if (best < 0) continue;

        results.push_back({app, best});
    }

    std::stable_sort(results.begin(), results.end(), [](const MatchResult& a, const MatchResult& b) {
        return a.score > b.score;
    });

    return results;
}

} // namespace nicoos
