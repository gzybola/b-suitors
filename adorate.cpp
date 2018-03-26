#include <iostream>
#include <set>
#include <sstream>
#include <iterator>
#include <thread>
#include <string>
#include <fstream>
#include <iostream>
#include <utility>
#include <algorithm>
#include <deque>
#include <numeric>
#include "blimit.hpp"
#include <atomic>
#include <vector>
#include <unordered_map>
#define UPDATE 128
#define MULTIPLY 9
#define ZERO 0

using namespace std;

typedef pair<unsigned, unsigned> para;

vector<unsigned long> staryNumer;

bool komparator (const para& a, const para& b) {
	if (a.first != b.first)
		return a.first > b.first;
	return staryNumer[a.second] > staryNumer[b.second];
}

class komparatorKlasa {
	public:
		bool operator() (const para& a,const para& b) const {
			return komparator(b, a);
		}
};

struct blokada {
	std::atomic_bool zatrzask;

	void lock() {
		bool wolny = false;
		while(!zatrzask.compare_exchange_weak(wolny, true, std::memory_order_acquire)) {
			wolny = false;
		}
	}
	void unlock() {
		zatrzask.store(false , std::memory_order_release);
	}
};

atomic<unsigned> wynik{0};
unsigned liczbaWatkow;
deque<blokada> muteksy;

vector<deque<para>> sasiedzi;
vector<deque<para>::iterator> skadZaczac;
vector<multiset<para, komparatorKlasa>> adoratorzy; 
vector<unsigned> kolejkaWierzcholkow;
vector<unsigned> kolejkaBvalue;
vector<unsigned> kolejkaDoPonownegoPrzetworzenia;
vector<unsigned> ileUporzadkowanychSasiadowWierzcholka;
vector<unsigned> ileNalezyPorzadkowac;
unordered_map<unsigned, unsigned>odrzuconeWierzcholki;

atomic<unsigned>* ostatniAdoratorNumer;
atomic<unsigned>* ostatniAdoratorWaga;
blokada zatrzymajOdrzucanie;
blokada kolejkaBlokada, atrapaBlokada, sasiedziBlokada, usunBlokada;

void blokuj(int u) {
	muteksy[u].lock();
}

void odblokuj(int u) {
	muteksy[u].unlock();
}

void przeczytajPlik(string sciezka) {
	ifstream input_stream;
	input_stream.open(sciezka, ifstream::in);
	string var;
	unordered_map<unsigned long, unsigned> nowaNumeracja;
	unsigned ile = 0;
	deque<para> x;
	while (input_stream.good()) {
		getline(input_stream, var);
		if (var.front() != '#' && var != "") {
			stringstream stream(var);
			unsigned long a, b;
			unsigned c;
			stream >> a; stream >> b; stream >> c;
			if (nowaNumeracja.count(a) == 0) {
				staryNumer.push_back(a);
				sasiedzi.push_back(x);
				nowaNumeracja[a] = ile;
				a = ile++;
			} else {
				a = nowaNumeracja[a];
			}

			if (nowaNumeracja.count(b) == 0) {
				staryNumer.push_back(b);
				sasiedzi.push_back(x);
				nowaNumeracja[b] = ile;
				b = ile++;
			} else {
				b = nowaNumeracja[b];
			}
			sasiedzi[a].push_back(make_pair(c, b));
			sasiedzi[b].push_back(make_pair(c, a));
		}
	}
}

long long mini (unsigned a, unsigned b) {
	return a > b ? b : a;
}

unsigned maxi(unsigned a, unsigned b) {
	return a > b ? a : b;
}

bool porownajKrawedzie(para a, para b, unsigned wierzcholek) {
	if (a.first != b.first)
		return a.first > b.first;
	return staryNumer[wierzcholek] > staryNumer[b.second];
}

void uporzadkujCzesciowo(unsigned wierzcholek) {
	deque<para> & it = sasiedzi[wierzcholek];
	unsigned ileSortowac = mini(ileNalezyPorzadkowac[wierzcholek], it.end() - skadZaczac[wierzcholek]);
	ileUporzadkowanychSasiadowWierzcholka[wierzcholek] += ileSortowac;

	partial_sort(skadZaczac[wierzcholek], skadZaczac[wierzcholek] + ileSortowac, it.end(), komparator);
}

void ilePorzadkowac(unsigned wierzcholek, unsigned blimit) {
	unsigned ile = 0;
	for (int i = 0 ; i <= blimit ; i++) {
		ile = maxi(bvalue(i, staryNumer[wierzcholek]) , ile);
	}
	ileNalezyPorzadkowac[wierzcholek] = mini(ile * MULTIPLY, sasiedzi[wierzcholek].size());
}

void wywolajWatki(vector<thread>* watki) {
	for (auto it = watki->rbegin() ; it != watki->rend() ; ++it) {
		it->join();
	}
	watki->clear();
}

void uporzadkujSasiada(unsigned wierzcholek, unsigned blimit) {
	skadZaczac[wierzcholek] = sasiedzi[wierzcholek].begin();
	ilePorzadkowac(wierzcholek, blimit);
	uporzadkujCzesciowo(wierzcholek);
}

void watekSasiedzi(unsigned blimit, vector<unsigned> & kolejka) {
	bool work = true;
	while (work) {
		sasiedziBlokada.lock();
		if (!kolejka.empty()) {
			unsigned ktoryWierzcholek = kolejka.back();
			kolejka.pop_back();
			sasiedziBlokada.unlock();
			uporzadkujSasiada(ktoryWierzcholek, blimit);
		}
		else {
			sasiedziBlokada.unlock();
			work = false;
		}
	}
}

void uporzadkujSasiadow(unsigned blimit) {
	vector<thread> watki;
	watki.reserve(liczbaWatkow);
	vector<unsigned> wierzcholki = vector<unsigned>(sasiedzi.size());
	iota(wierzcholki.begin(), wierzcholki.end(), 0);
	ileUporzadkowanychSasiadowWierzcholka = vector<unsigned>(sasiedzi.size());
	ileNalezyPorzadkowac = vector<unsigned>(sasiedzi.size());
	skadZaczac = vector<deque<para>::iterator>(sasiedzi.size());

	for (unsigned i = 1 ; i < liczbaWatkow ; i++) {
		watki.push_back(thread {watekSasiedzi, blimit, ref(wierzcholki)});
	}
	watekSasiedzi(blimit, ref(wierzcholki));
	wywolajWatki(&watki);
}

void stworzPoczatkowaKolejkeZarezerwujMiejsceDlaAdoratorow() {
	ostatniAdoratorWaga = new atomic<unsigned>[sasiedzi.size()];
	ostatniAdoratorNumer = new atomic<unsigned>[sasiedzi.size()];	
	muteksy = deque<blokada>(sasiedzi.size());
	adoratorzy = vector<multiset<para, komparatorKlasa>>(sasiedzi.size());

	for (unsigned i = 0 ; i < sasiedzi.size() ; i++) {
		kolejkaWierzcholkow.push_back(i);
	}
}

void stworzAtrapyAdoratorow(unsigned wierzcholek, unsigned ktory) {
	vector<para> tmp = vector<para>(kolejkaBvalue[ktory], make_pair(ZERO, ZERO));
	ostatniAdoratorWaga[wierzcholek] = ZERO;
	ostatniAdoratorNumer[wierzcholek] = ZERO;
	adoratorzy[wierzcholek].insert(tmp.begin(), tmp.end());
}

void stworzAtrapyAdoratorowWatek(vector<unsigned> & wierzcholki) {
	bool pracuj = true;
	while (pracuj) {
		atrapaBlokada.lock();
		if (!wierzcholki.empty()) {
			unsigned wierzcholek = wierzcholki.back();
			unsigned ktory = wierzcholki.size() - 1;
			wierzcholki.pop_back();
			atrapaBlokada.unlock();
			stworzAtrapyAdoratorow(wierzcholek, ktory);
		}
		else {
			atrapaBlokada.unlock();
			pracuj = false;
		}
	}
}

void stworzAtrapyAdoratorow() {
	vector<thread> t;
	t.reserve(liczbaWatkow);
	vector<unsigned> wierzcholki = vector<unsigned>(kolejkaWierzcholkow);

	for (int i = 1 ; i < liczbaWatkow ; i++) {
		t.push_back(thread{stworzAtrapyAdoratorowWatek, ref(wierzcholki)});
	}

	stworzAtrapyAdoratorowWatek(ref(wierzcholki));
	wywolajWatki(&t);
}

void stworzKolejke() {
	for (auto it = odrzuconeWierzcholki.begin() ; it != odrzuconeWierzcholki.end() ; ++it) {
		kolejkaDoPonownegoPrzetworzenia.push_back(it->first);
		kolejkaBvalue.push_back(it->second);
	}

	odrzuconeWierzcholki.clear();
}

void aktualizujOdrzucone (unordered_map<unsigned, unsigned> & odrzuceni) {
	zatrzymajOdrzucanie.lock();
	for (auto it = odrzuceni.begin() ; it != odrzuceni.end() ; ++it) {
		odrzuconeWierzcholki[it->first] += it->second;
	}

	zatrzymajOdrzucanie.unlock();
	odrzuceni.clear();
}

void odrzucSasiada(unsigned wierzcholek) {
	skadZaczac[wierzcholek]++;
	ileUporzadkowanychSasiadowWierzcholka[wierzcholek]--;
	if (ileUporzadkowanychSasiadowWierzcholka[wierzcholek] == 0 && skadZaczac[wierzcholek] != sasiedzi[wierzcholek].end()) {
		uporzadkujCzesciowo(wierzcholek);
	}
}

bool adoruj(para sasiad, unsigned wierzcholek, unordered_map<unsigned, unsigned> & odrzuceniAdoratorzy) {
	blokuj(sasiad.second);

	if (porownajKrawedzie(sasiad, *adoratorzy[sasiad.second].begin(), wierzcholek)) {
		//adoruje i ustawia najmniejszego adoratora
		para usunietyAdorator = *adoratorzy[sasiad.second].begin();
		adoratorzy[sasiad.second].erase(adoratorzy[sasiad.second].begin());
		adoratorzy[sasiad.second].insert(make_pair(sasiad.first, wierzcholek));
		ostatniAdoratorNumer[sasiad.second] = adoratorzy[sasiad.second].begin()->second;
		ostatniAdoratorWaga[sasiad.second] = adoratorzy[sasiad.second].begin()->first;

		wynik += sasiad.first;

		//oznajmia bylyemu adoratorowi KONIEC
		if (usunietyAdorator.first != 0) {
			odrzuceniAdoratorzy[usunietyAdorator.second]++;
			wynik -= usunietyAdorator.first;

			if (odrzuceniAdoratorzy.size() == UPDATE)
				aktualizujOdrzucone(ref(odrzuceniAdoratorzy));
		}
		odblokuj(sasiad.second);
		return true;
	}
	odblokuj(sasiad.second);
	return false;
}

void szukajWierzcholkowDoAdorowania(unsigned wierzcholek, unsigned ileAdoratorow) {
	unordered_map<unsigned, unsigned> odrzuceniAdoratorzy;
	int i = 1;
	while (i <= ileAdoratorow && skadZaczac[wierzcholek] != sasiedzi[wierzcholek].end()) {
		auto sasiad = skadZaczac[wierzcholek];
		unsigned tmpWaga = ostatniAdoratorWaga[sasiad->second];
		unsigned tmpNumer = ostatniAdoratorNumer[sasiad->second];
		para tmpAdorator = make_pair(tmpWaga, tmpNumer);

		while (sasiad != sasiedzi[wierzcholek].end() && !porownajKrawedzie(*sasiad, tmpAdorator,  wierzcholek)) {			
			odrzucSasiada(wierzcholek);
			sasiad = skadZaczac[wierzcholek];
			unsigned tmpWaga = ostatniAdoratorWaga[sasiad->second];
			unsigned tmpNumer = ostatniAdoratorNumer[sasiad->second];
			tmpAdorator = make_pair(tmpWaga, tmpNumer);
		}

		if (sasiad != sasiedzi[wierzcholek].end()) {
			if (adoruj(*sasiad, wierzcholek, ref(odrzuceniAdoratorzy))) {
				i++;
			}
			odrzucSasiada(wierzcholek);
		}
	}

	if (odrzuceniAdoratorzy.size() > 0)
		aktualizujOdrzucone(ref(odrzuceniAdoratorzy));
}

void stworzBKolejke(unsigned method) {
	for (auto it = kolejkaWierzcholkow.begin() ; it != kolejkaWierzcholkow.end() ; ++it) {
		kolejkaBvalue.emplace_back(bvalue(method, staryNumer[*it]));
	}
}

void przywrocSasiadowJednegoWierzcholka(unsigned numer) {
	ileUporzadkowanychSasiadowWierzcholka[numer] += skadZaczac[numer] - sasiedzi[numer].begin();
	skadZaczac[numer] = sasiedzi[numer].begin();
}

void watekPrzywrocSasiadow(unsigned ktory) {
	for (int i = ktory ; i < sasiedzi.size() ; i += liczbaWatkow)
		przywrocSasiadowJednegoWierzcholka(i);
}

void przywrocSasiadow() {
	vector<thread> t;
	t.reserve(liczbaWatkow - 1);
	for (unsigned k = 1 ; k < liczbaWatkow ; k++) {
		t.push_back(thread{watekPrzywrocSasiadow, k});
	}
	watekPrzywrocSasiadow(ZERO);
	wywolajWatki(&t);
}

void usunAdoratorowWatek(vector<unsigned> & kolejka) {
	bool pracuje = true;
	while(pracuje) {
		usunBlokada.lock();
		if(!kolejka.empty()) {
			unsigned wierzcholek = kolejka.back();
			kolejka.pop_back();
			usunBlokada.unlock();
			adoratorzy[wierzcholek].clear();
		}
		else {
			usunBlokada.unlock();
			pracuje = false;
		}
	}
}

void usunAdoratorow() {
	vector<thread> t;
	t.reserve(liczbaWatkow - 1);
	vector<unsigned> kolejka = vector<unsigned>(kolejkaWierzcholkow);
	for (unsigned it = 1 ; it < liczbaWatkow ; it++) {
		t.push_back(thread{usunAdoratorowWatek, ref(kolejka)});
	}
	usunAdoratorowWatek(ref(kolejka));
	wywolajWatki(&t);
}

void watekPrzetworzKolejke(unsigned ktory, vector<unsigned> * kolejka) {
	bool work = true;
	while (work) {
		kolejkaBlokada.lock();
		if (!kolejka->empty()) {
			unsigned wierzcholek = kolejka->back();
			unsigned ktory = kolejkaBvalue.back();
			kolejka->pop_back();
			kolejkaBvalue.pop_back();
			kolejkaBlokada.unlock();
			szukajWierzcholkowDoAdorowania(wierzcholek, ktory);
		}
		else {
			kolejkaBlokada.unlock();
			work = false;
		}
	}
}

void przetworzKolejke(vector<unsigned>* kolejka) {
	vector<thread> t;
	t.reserve(liczbaWatkow);
	for (unsigned k = 1 ; k < liczbaWatkow ; k++) {
		t.push_back(thread (watekPrzetworzKolejke, k, kolejka));
	}
	watekPrzetworzKolejke(ZERO, kolejka);
	wywolajWatki(&t);
}

int main(int argc, char* argv[]) {
	ios_base::sync_with_stdio(false);
	if (argc != 4) {
		std::cerr << "usage: "<<argv[0]<<" thread-count inputfile b-limit"<< std::endl;
		return 1;
	}
	liczbaWatkow = stoi(argv[1]);
	unsigned bLimit = stoi(argv[3]);
	przeczytajPlik(argv[2]);
	uporzadkujSasiadow(bLimit);
	stworzPoczatkowaKolejkeZarezerwujMiejsceDlaAdoratorow();
	unsigned licznik = 0;
	for (unsigned i = 0 ; i <= bLimit ; i ++) {
		stworzBKolejke(i);
		stworzAtrapyAdoratorow();
		vector<unsigned> kolejka = vector<unsigned> (kolejkaWierzcholkow);
		przetworzKolejke(&kolejka);
		stworzKolejke();
		while (!kolejkaDoPonownegoPrzetworzenia.empty()) {
			przetworzKolejke(&kolejkaDoPonownegoPrzetworzenia);
			stworzKolejke();
		}
		cout << wynik/2 << endl;
		wynik = 0;
		if (i != bLimit) {
			przywrocSasiadow();
			usunAdoratorow();
		}
	}
	delete [] ostatniAdoratorWaga;
	delete [] ostatniAdoratorNumer;
	return 0;
}
