/*Program in SWI-Prolog*/
/*name, min, max*/
ageGroup("younge", 18, 30).
ageGroup("old", 30, 60).
/*name, age, departament, livePlace, work date, children?, maried?*/
person("Vasay", 20, "it", "lenina,5","12.12.2012",0,0).
person("Sasha", 25, "it", "Korolenko,12","10.10.2010",0,1).
person("Sergey", 55, "it", "prospect,21","25.07.2000",1,1).
person("Anna", 26, "Buh", "pobeda,39","07.01.2005",1,0).
person("Natasha", 19, "Buh", "kirova,48","25.02.2008",0,1).
person("Anastasiya", 35, "Buh", "prospect,21","30.05.1999",1,1).
person("Elena", 29, "OR", "prospect,120","20.08.2000",0,0).
person("Lisa", 43, "OR", "lenina,210","04.09.1991",0,1).
person("Aleksei", 50, "OR", "korolenko,13","02.07.1989",0,0).

:- write("Expert system"), nl, write("Read queryDepAge or querySocial"), nl, write("Example query"), nl, write("Read queryDepAge(_,\"it\",\"younge\").").

queryDepAge(Name, Dep, AgeMin, AgeMax) :- person(Name, X, Dep, _, _, _, _), X>AgeMin, X<AgeMax.
queryDepAge(Name, Dep, AgeGroup) :- ageGroup(AgeGroup, Min, Max), queryDepAge(Name, Dep, Min, Max), write(Name), write(", "), write(Dep), write(".").
querySocial(WorkData, LivePlace, Children, Maried) :- person(Name, Age, Dep, LivePlace, WorkData, Children, Maried), write(Name), write(", "), write(Age), write(", "), write(Dep), write(".").
