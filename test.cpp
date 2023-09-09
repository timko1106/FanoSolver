#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using std::cout;
using std::fstream;
using std::string;
using std::vector;

//Набор состояний вершины графа
enum state : char {
	PSEUDO = '-',
	ROOT = '~',
	BUSY = '='
};

struct Node {
	char value;
	int depth;
	Node* nodes[2] = {};
	Node* parent;
	//Конструктор по умолчанию
	Node (char name, Node* parent = nullptr) : value (name), parent(parent) {
		nodes[0] = nullptr;
		nodes[1] = nullptr;
		if (parent != nullptr)depth = parent->depth + 1;
		else depth = 0;
	}
	//Конструктор копирования (parent для рекурсивного копирования)
	Node (const Node& node, Node *parent = nullptr) : value (node.value), depth (node.depth), parent (parent) {
		nodes[0] = nullptr;
		nodes[1] = nullptr;
		if (node.nodes[0])nodes[0] = new Node (*(node.nodes[0]), this);
		if (node.nodes[1])nodes[1] = new Node (*(node.nodes[1]), this);
	}
	//Деструктуризация
	~Node () {
		if (nodes[0])delete nodes[0];
		if (nodes[1])delete nodes[1];
	}
	bool isreal () const {
		return value != PSEUDO;
	}
	bool isused () const {
		return value == BUSY;
	}
	bool isroot () const {
		return value == ROOT;
	}
};
using node = Node*;
//Бинарное дерево (2 ветки всегда)
class tree {
	node head;
	//Рекурсивное получение вершины по имени
	const Node* const get (char name, node from) const {
		if (from == nullptr)return nullptr;
		if (name == from->value)return from;
		const Node* const fromleft = get (name, from->nodes[0]);
		if (fromleft != nullptr)return fromleft;
		const Node* const fromright = get(name, from->nodes[1]);
		return fromright;
	}
public:
	//Конструктор обычный
	tree () : head(new Node (ROOT)) {}
	//head не должен быть nullptr
	tree (const tree& _tree) : head (new Node (*(_tree.head))) {}
	~tree () {delete head;}
	node get (char name) {
		return const_cast<node>(get(name, head)); 
	}
	const Node* const get (char name) const { 
		return get(name, head); 
	}
	//Получение корня дерева
	node getroot () { return head; }
	const Node* const getroot() const { 
		return head; 
	}
};
void print_tree (const tree& _tree);
const int depth_level = 4;

int adds_count = 0;//Приблизительное количество вариантов подстановки 1 буквы
int searched = 0;//Кол-во исследованных вариантов

tree *besttree = nullptr;
int best_result = 1000000;

//Добавление псевдовершин рекурсивно (мест, где можно поставить вершину)
void adding (node node, int depth = 0) {
	if (node == nullptr) {
		++adds_count;
		return;
	}
	if (depth + 1 == depth_level)return;
	for (int i = 0; i < 2; ++i) {
		if (node->nodes[i] == nullptr) {
			node->nodes[i] = new Node(PSEUDO, node);
			++adds_count;
			adding (node->nodes[i], depth + 1);
		}
		if (node->nodes[i]->isused())adding (node->nodes[i], depth);
	}
}
tree add_fakes (tree& origin) {
	tree fake (origin);
	adding(fake.getroot());
	adds_count = adds_count * 206 / 100;//Компенсация нехватающих вариантов
	return fake;
}
//Попытки подстановки буквы. Рекурсивно ищет по всему дереву ветви, не занятые существующими буквами.
//Никак не оптимизировано.
//apply - тип лямбды
//func принимает дерево и возвращает ничего (будет продолжать добавлять буквы или проверит дерево)
template<typename apply>
void deep_letter_substitution (char c, tree& curtree, apply func, node from = nullptr) {
	if (from == nullptr) from = curtree.getroot();
	for (int i = 0; i < 2; ++i) {
		node old = from->nodes[i];//пытаемся заменить на букву
					  //Если вершина ничем не занята (т.е. nullptr) или является потенциальным местом - вставляем
		if (old == nullptr || !(old->isreal())) {
			if (old == nullptr) from->nodes[i] = new Node(c, from);
			else old->value = c;
			tree deepcopied = curtree;
			node parent = deepcopied.get(c);//изменяем КОПИЮ (не оригинал)
			while (!(parent->isroot())) {
				if (!(parent->isreal()))parent->value = BUSY;
				parent=parent->parent;
			}
			func (deepcopied);
			if (old == nullptr) {
				delete from->nodes[i];
				from->nodes[i] = nullptr;
			}
			else {
				old->value = PSEUDO;
				deep_letter_substitution (c, curtree, func, old);
			}
		} else if (old->isused()) deep_letter_substitution (c, curtree, func, old);//Если занята вершина, 
											   //не гарантировано занята ветвь (нужно поискать).
	}
}
const int NONCOUNTED = -1;
//Глубокая подстановка всех символов
//Для поиска лучшего дерева (очень глубокая рекурсия)
void deep_substitution(string::iterator curr, const string& all, const string& shortest_code, tree& curtree) {
	if (curr == all.end()) return;//Даже перешли конец -> ошибка
	if (curr == all.end() - 1) {//Если является последним символом
		auto lambda = [&shortest_code](const tree& _tree) {
					const int ASCII_MODIFIED_SIZE = 256;
					int* array = new int[ASCII_MODIFIED_SIZE];
					int count = 0;
					for (int i = 0; i < ASCII_MODIFIED_SIZE; ++i)array[i] = NONCOUNTED;
					for (char c : shortest_code) {
						if (array[c] == NONCOUNTED)array[c] = _tree.get (c)->depth;
						count += array[c];
					}
					if (count < best_result) {
						//print_tree (_tree);
						best_result = count;
						delete besttree;
						besttree = new tree(_tree);
					}
					delete[] array;
				};
		deep_letter_substitution (*curr, curtree, lambda);
		return;
	}
	auto lambda = [curr, &all, &shortest_code](tree& _tree) {
			if (curr == all.begin()) {
				++searched;
				if (searched <= adds_count)
					cout << (searched * 100 / adds_count) << "% done\n";
			}
			deep_substitution (curr + 1, all, shortest_code, _tree);
			};
	deep_letter_substitution (*curr, curtree, lambda);
}

//Вывод вершины
void print_node (const Node* const node, vector<bool>& vals) {
	if (!(!(node->isreal()) || node->isused() || node->isroot())) {//Мнимые, промежуточные и корень не выводим
		cout << node->value << ' ';
		for (bool v : vals)cout << v;
		cout << '\n';
	}
	for (int i = 0; i < 2; ++i) {
		if (node->nodes[i]) {//Очевидно, что nullptr выводить не хотим
			vals.push_back (i);
			print_node (node->nodes[i], vals);
		}
	}
	vals.pop_back();
}
//Вывод всего дерева
void print_tree (const tree& _tree) {
	vector<bool> v;
	print_node (_tree.getroot(),v);
}

//Ищется дерево УДОВЛЕТВОРЯЮЩЕЕ условию Фано (обычному, для обратного сами инвертируйте дерево)
int main (int argc, char **argv) {
	if (argc < 2)return 1;//argv[0]=путь к исполняемому файлу, argv[1]=путь к дереву
	fstream stream (argv[1], std::ios_base::in);//Открытие потока
	if (!(stream.is_open()))return 1;
	cout << "Opened\n";
	tree _tree;
	//Ввод оригинального дерева
	int count;
	stream >> count;
	node head = _tree.getroot();
	for (int i = 0; i < count; ++i) {
		node curr = head;
		char name;
		string mask;
		stream >> name >> mask;
		auto begin = mask.begin();
		auto end = mask.end() - 1;
		for (auto _iter = begin; _iter != end; ++_iter) {
			char idx = (*_iter) - '0';
			if (curr->nodes[idx] == nullptr)curr->nodes[idx] = new Node (BUSY, curr);//Промежуточные вершины
			curr = curr->nodes[idx];
		}
		curr->nodes[(*end) - '0'] = new Node (name, curr);//Создание самой вершины
	}
	//Ввод нужных вершин и последовательности кратчайшей длины
	string all_needed, shortest_code;
	stream >> all_needed >> shortest_code;
	stream.close();
	cout << "Closed\n";
	//print_tree (_tree);
	//return 0;
	cout << "Processing...\n";
	{
		tree with_fakes = add_fakes(_tree);
		deep_substitution (all_needed.begin(), all_needed, shortest_code, with_fakes);
		cout << "Done.\n";
		cout << best_result << '\n';
		if (besttree) {//Может и не найтись дерева
			print_tree(*besttree);
			delete besttree;
		}
	}
	return 0;
}
