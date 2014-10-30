
#include "solveperiodambiguity.h"
#include <FactExtract/Parser/common/inputitem.h>

#include <FactExtract/Parser/afdocparser/rusie/factgroup.h>

struct CPeriodSolutionWeight
{
    yvector<size_t>        m_Solution;
    CWeightSynGroup        m_VirtualGroup;
    long                   m_Coverage;
    CInputItem::TWeight    m_Weight;       //cached from m_VirtualGroup.GetWeight()
    CPeriodSolutionWeight();
    CPeriodSolutionWeight& operator += (const  CPeriodSolutionWeight& _X);
    bool  operator < (const  CPeriodSolutionWeight& _X) const;
    bool  operator == (const  CPeriodSolutionWeight& _X) const;

    void AddOccurrence(const COccurrence& occurrence, size_t index);
};

CPeriodSolutionWeight::CPeriodSolutionWeight()
{
    //m_Weight = 1;
    m_Coverage = 0;
    m_VirtualGroup.m_KwtypesCount = 0;

    m_Weight = CInputItem::TWeight::Zero();
};

//CPeriodSolutionWeight&    CPeriodSolutionWeight::operator += (const  CPeriodSolutionWeight& _X)
//{
//    //m_Weight    *= _X.m_Weight;
//    m_VirtualGroup.AddWeightOfChild(&_X.m_VirtualGroup);
//    m_Coverage    += _X.m_Coverage;
//    m_PeriodsCount += _X.m_PeriodsCount;
//    return *this;
//};

bool  CPeriodSolutionWeight::operator < (const  CPeriodSolutionWeight& _X) const
{
    if (m_Coverage !=  _X.m_Coverage)
        return m_Coverage < _X.m_Coverage;
    else {
/*        double w1;
                m_VirtualGroup.GetItemWeight(w1);
        double w2;
                _X.m_VirtualGroup.GetItemWeight(w2);
        if (w1 != w2)
            return w1 < w2;
*/
        if (m_Weight != _X.m_Weight)
            return m_Weight < _X.m_Weight;
        else
            return m_Solution.size() > _X.m_Solution.size(); // две группы хуже, чем одна
    }

}

bool  CPeriodSolutionWeight::operator == (const CPeriodSolutionWeight& _X) const
{
/*
   double weight1;
   m_VirtualGroup.GetItemWeight(weight1);
   double weight2;
   _X.m_VirtualGroup.GetItemWeight(weight2);

   return m_Coverage == _X.m_Coverage && weight1 == weight2;
*/
    return m_Coverage == _X.m_Coverage && m_Weight == _X.m_Weight;
}

void CPeriodSolutionWeight::AddOccurrence(const COccurrence& occurrence, size_t index)
{
    size_t coverage = 0;
    if (occurrence.m_pInputItem != NULL) {
        m_VirtualGroup.SetLastWord(occurrence.second - 1);
        m_VirtualGroup.AddWeightOfChild(occurrence.m_pInputItem);
        m_Weight = m_VirtualGroup.GetWeight(); //cache weight!
        coverage = occurrence.m_pInputItem->GetCoverage();
    }
    if (coverage == 0)
        coverage = occurrence.second - occurrence.first;
    m_Coverage += coverage;
    m_Solution.push_back(index);
}

/*
SolveAmbiguityRecursive finds the longest non-intersected set of periods,
which  includes period A.

This function recieves the following:
1. A base set of periods  "Occurrences", which have intersections.
2. A position in the set of periods "Start", from which an ambiguous ( = with intersections)
part starts;


The output parameter are:
1.  "Solution", which is the maximal possible unambiguous subset of
Occurrences that includes period Start.
2.  "MaxRightBorder" is  the maximal right border, to which extends the ambigous set of periods.
MaxRightBorder can be
    1. the end of the whole text;
    2. the start of the first unambiguous period;
    3. the point which doesn't belong to any period.
The return value is coverage of this maximal solution i.e.
length(Solution[0]) + ... length(Solution[Solution.size() - 1])
*/

void SolveAmbiguityRecursive (const yvector< COccurrence >& Occurrences, size_t Start, CPeriodSolutionWeight& Solution, size_t& MaxRightBorder)
{
    // if this period has no intersection, then exit
    if (!Occurrences[Start].m_bAmbiguous) {
        return;
    };

    // find the first period, which intersects with period Start, say First, and collect
    // all periods which have intersection with intersection of Start and First.
    // Therefore we find the maximal subset which includes period Start
    // and each two members of this subset have an intersection.
    yvector<size_t> InconsistentPeriods;

    // "EndIntersection" is the end of the common intersection, it is not
    // obligatory to check the start point of the common intersection due the order in the periods
    size_t EndIntersection = Occurrences[Start].second;

    for (size_t i=Start; i < Occurrences.size(); i++) {
        if (Occurrences[i].first < EndIntersection) {
            InconsistentPeriods.push_back(i);
            // initializing  EndIntersection
            EndIntersection = std::min(Occurrences[Start].second, Occurrences[i].second);
            if (MaxRightBorder <  Occurrences[i].second)
                MaxRightBorder = Occurrences[i].second;

        } else
            break;
    };

    /*
        going through all possible variants of continuing this  solution,
        i.e. cycling through the   InconsistentPeriods.
    */

    CPeriodSolutionWeight SaveSolution = Solution;

    for (size_t i=0; i < InconsistentPeriods.size(); i++) {
        size_t  k = InconsistentPeriods[i];

        CPeriodSolutionWeight CurrSolution = SaveSolution;
        CurrSolution.AddOccurrence(Occurrences[k], k);
/*
        size_t CurrCoverage = 0;
        if (Occurrences[k].m_pInputItem != 0)
        {
            CurrSolution.m_VirtualGroup.SetLastWord( Occurrences[k].second-1);
            CurrSolution.m_VirtualGroup.AddWeightOfChild( Occurrences[k].m_pInputItem );
            CurrSolution.m_VirtualGroup.GetItemWeight(CurrSolution.m_Weight);       //cache weight;
            CurrCoverage = Occurrences[k].m_pInputItem->GetCoverage();
        }
        if (CurrCoverage == 0)
            CurrCoverage = Occurrences[k].second - Occurrences[k].first;

        CurrSolution.m_Coverage += CurrCoverage;

        // try k as a possible  variant of continuing
        CurrSolution.m_Solution.push_back(k);
*/

        //  ignoring  all periods which have  the "left" intersection with  period k (==inconsistent with with  period)
        // , i.e. they have intersection and the start point
        // of the  period k is less or equal than the start point these periods.
        for (size_t j=k+1; j < Occurrences.size(); j++)
            if (Occurrences[j].first >=  Occurrences[k].second
                &&    (Occurrences[j].first < MaxRightBorder)
                ) {
                // The first non-intersected period is found,
                // we should include it to current solution and get
                // the  coverage  of  this variant
                SolveAmbiguityRecursive(Occurrences, j,  CurrSolution, MaxRightBorder);
                break;
            };

        if    (Solution < CurrSolution)
            Solution = CurrSolution;

    };
};

void InitAmbiguousSlot(yvector< COccurrence >& Occurrences)
{
    /* quite slow implementation. could be done with single iteration through sorted array - i.e. in O(n), not O(n^2) */
    for (size_t i=0; i < Occurrences.size(); i++)
        Occurrences[i].m_bAmbiguous = false;

    for (size_t i=0; i < Occurrences.size(); i++) {
        for (size_t k=i+1; k < Occurrences.size(); k++)
            if (Occurrences[k].first < Occurrences[i].second) {
                Occurrences[k].m_bAmbiguous = true;
                Occurrences[i].m_bAmbiguous = true;
            } else
                break;
    }
}

void SolveAmbiguity_Old(yvector< COccurrence >& Occurrences)
{
    //previous version of disambiguation - it is quite slower in worst cases

    sort (Occurrences.begin(), Occurrences.end());
    InitAmbiguousSlot(Occurrences);
    //int CountOfAmbigousPlaces = 0;
    for (size_t i = 0; i + 1 < Occurrences.size(); ++i)
        if    (Occurrences[i + 1].first < Occurrences[i].second) {
            CPeriodSolutionWeight Solution;
            Solution.m_VirtualGroup.SetPair(Occurrences[i].first,Occurrences[i].first);

            size_t MaxRightBorder = Occurrences[i].second;
            SolveAmbiguityRecursive(Occurrences,i, Solution, MaxRightBorder);

            assert (!Solution.m_Solution.empty());

            int j=Solution.m_Solution.back() + 1;

            for (; j < (int)Occurrences.size(); ++j)
                if (Occurrences[j].first >= Occurrences[Solution.m_Solution.back()].second)
                    break;

            Occurrences.erase(Occurrences.begin() + Solution.m_Solution.back() + 1, Occurrences.begin() + j);

            for (size_t k=Solution.m_Solution.size() - 1; k > 0; --k) {
                size_t start = Solution.m_Solution[k - 1] + 1;
                size_t end = Solution.m_Solution[k];
                Occurrences.erase(Occurrences.begin() + start, Occurrences.begin() + end);
            };
            Occurrences.erase(Occurrences.begin() + i, Occurrences.begin() + Solution.m_Solution[0]);
            //CountOfAmbigousPlaces++;
        };
    //printf ("Count Of Ambiguous Places is %i\n",CountOfAmbigousPlaces);;

};

bool SortByRightBorderPredicate(const COccurrence& o1, const COccurrence& o2)
{
    if (o1.second == o2.second)
        return o1.first < o2.first;
    else
        return o1.second < o2.second;
}

void SolveAmbiguity(yvector<COccurrence>& Occurrences, yvector<size_t>& res)
{
    if (Occurrences.size() == 0)
        return;

    if (Occurrences.size() == 1) {
        res.push_back(0);
        return;
    }

    std::sort(Occurrences.begin(), Occurrences.end(), SortByRightBorderPredicate);

    yvector<CPeriodSolutionWeight> solutions(Occurrences.size());
    for (size_t i = 0; i < Occurrences.size(); ++i) {
        const COccurrence& cur_occurrence = Occurrences[i];
        const CPeriodSolutionWeight* best_overlapping_solution = NULL;
        const CPeriodSolutionWeight* best_non_overlapping_solution = NULL;
        for (int j = i - 1; j >= 0; --j)
            if (Occurrences[j].second <= cur_occurrence.first)       //does not intersect
            {
                best_non_overlapping_solution = &(solutions[j]);
                break;
            } else if (best_overlapping_solution == NULL || *best_overlapping_solution < solutions[j])
                best_overlapping_solution = &(solutions[j]);

        CPeriodSolutionWeight& cur_solution = solutions[i];
        if (best_non_overlapping_solution == NULL)
            cur_solution.m_VirtualGroup.SetPair(cur_occurrence.first, cur_occurrence.first);
        else
            cur_solution = *best_non_overlapping_solution;     // nasty copying!
        cur_solution.AddOccurrence(cur_occurrence, i);

        //if overlapped solution is still better
        if (best_overlapping_solution != NULL && cur_solution < *best_overlapping_solution)
            cur_solution = *best_overlapping_solution;           // nasty copying again!
    }
    CPeriodSolutionWeight& best_solution = solutions.back();
    res.swap(best_solution.m_Solution);
}

void SolveAmbiguity(yvector<COccurrence>& Occurrences)
{
    yvector<size_t> res;
    SolveAmbiguity(Occurrences, res);
    if (res.size() != Occurrences.size()) {
        yvector<COccurrence> tmp(res.size());
        for (size_t i = 0; i < res.size(); ++i)
            tmp[i] = Occurrences[res[i]];
        Occurrences.swap(tmp);
    }
}

void SolveAmbiguity(yvector<COccurrence>& Occurrences, yvector<COccurrence>& DroppedOccurrences)
{
    yvector<size_t> res;
    SolveAmbiguity(Occurrences, res);
    if (res.size() != Occurrences.size()) {
        std::sort(res.begin(), res.end());
        yvector<COccurrence> tmp(res.size());
        for (size_t i = 0; i < res.size(); ++i)
            tmp[i] = Occurrences[res[i]];

        size_t next_res_index = 0;
        res.push_back(Occurrences.size());
        for (size_t j = 0; j < Occurrences.size(); ++j)
            if (j == res[next_res_index])
                next_res_index += 1;
            else
                DroppedOccurrences.push_back(Occurrences[j]);

        Occurrences.swap(tmp);
    }
}

