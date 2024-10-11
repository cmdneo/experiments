#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#include <optional>
#include <random>

#include "gen_bench.hxx"

using std::cout;

class AVLTree;

struct Node {
	explicit Node(int d)
		: key(d)
	{
	}

	int balance_factor() const
	{
		auto l = left ? left->height : -1;
		auto r = right ? right->height : -1;
		return l - r;
	}

	void update_height()
	{
		height =
			std::max(left ? left->height : -1, right ? right->height : -1) + 1;
	}

	int key = 0;
	int height = 0;
	std::unique_ptr<Node> left = nullptr;
	std::unique_ptr<Node> right = nullptr;
	Node *parent = nullptr;
};

// Pretty printers :)
//-------------------------------------------------------------------
std::ostream &
print_node_conn_line(std::ostream &os, const Node *parent, bool for_left)
{
	// UTF-8 is broken in C++ so use vector instead,
	// because we need to print in reversed direction & only 2 distinct chars
	std::vector<bool> bar_or_space; // vert-bar: true, space: false
	while (auto sparent = parent->parent) {
		bar_or_space.insert(bar_or_space.end(), 3, false); // 3 spaces
		// If left node, then bar as right node is below left node
		bar_or_space.push_back(sparent->left.get() == parent);
		parent = sparent;
	}

	std::for_each(
		bar_or_space.crbegin(), bar_or_space.crend(),
		[&os](bool bar) { os << (bar ? "│" : " "); }
	);
	os << (for_left ? "├───" : "└───");

	return os;
}

std::ostream &print_node(std::ostream &os, const Node *node)
{

	if (node) {
		os << "[" << node->key << "](" << node->balance_factor() << ")\n";
	} else {
		os << "[]\n";
		return os;
	}
	// If no child nodes
	if (!node->left && !node->right)
		return os;

	print_node_conn_line(os, node, true);
	print_node(os, node->left.get());
	print_node_conn_line(os, node, false);
	print_node(os, node->right.get());

	return os;
}
//-------------------------------------------------------------------

// Rotate left about X
//     X                Y
//   /  \              / \
//  a    Y     =>     X   c
//      / \          / \
//     b   c        a  b
void rotate_left(std::unique_ptr<Node> &x)
{
	assert(x->right);
	auto y = std::move(x->right);

	// Update backreferences
	y->parent = x->parent;
	x->parent = y.get();
	if (y->left)
		y->left->parent = x.get();

	// Rotate left
	x->right = std::move(y->left);
	y->left = std::move(x);
	x = std::move(y); // Put y where x was
	x->left->update_height();
	x->update_height();
}

// Rotate right about X
//       X            Y
//     /  \          / \
//    Y    a   =>   c   X
//   / \               / \
//  c   b             b   a
void rotate_right(std::unique_ptr<Node> &x)
{
	assert(x->left);
	auto y = std::move(x->left);

	// Update backreferences
	y->parent = x->parent;
	x->parent = y.get();
	if (y->right)
		y->right->parent = x.get();

	// Rotate right
	x->left = std::move(y->right);
	y->right = std::move(x);
	x = std::move(y); // Put y where x was
	x->right->update_height();
	x->update_height();
}

class AVLTree
{
public:
	AVLTree() = default;

	Node &insert(int val)
	{
		// If tree empty, then add root node
		if (!tree) {
			tree.reset(new Node(val));
			return *tree;
		}
		return insert_impl(tree, val);
	}

	bool delete_key(int key);

	Node *search(int val) { return search_impl(tree, val); }

	Node *get_root() const { return tree.get(); }

	void clear() { tree.reset(nullptr); }

private:
	std::unique_ptr<Node> tree;

	static Node &insert_impl(std::unique_ptr<Node> &nd, int val);
	Node *search_impl(std::unique_ptr<Node> &nd, int val);
	/// Just balances the current node, use recursively from bottom to top
	static void balance_node(std::unique_ptr<Node> &nd);
};

bool AVLTree::delete_key(int key)
{
	auto node = search(key);
	if (node == nullptr)
		return false;

	// Get the unique pointer associated with the node,
	// as search() returns a raw pointer.

	// 0 Children
	if (node->left == nullptr && node->left == nullptr) {
	}
	// Left children exists
	else if (node->left != nullptr) {
	}
	// Right children exists
	else if (node->right != nullptr) {
	}
	// Both(left and right children exist)
	else {
	}

	return true;
}

Node &AVLTree::insert_impl(std::unique_ptr<Node> &nd, int val)
{
	assert(nd);
	auto &ins = val < nd->key ? nd->left : nd->right;

	if (ins) {
		auto &ret = insert_impl(ins, val);
		nd->update_height();
		balance_node(nd);
		return ret;
	} else {
		ins.reset(new Node(val));
		ins->parent = nd.get();
		nd->update_height();
		balance_node(nd);
		return *ins;
	}
}

Node *AVLTree::search_impl(std::unique_ptr<Node> &node, int val)
{
	if (!node)
		return nullptr;
	if (node->key == val)
		return node.get();
	return search_impl(val < node->key ? node->left : node->right, val);
}

void AVLTree::balance_node(std::unique_ptr<Node> &nd)
{

	if (std::abs(nd->balance_factor()) <= 1)
		return;

	// Four cases total
	// Left side unbalanced
	if (nd->balance_factor() > 0) {
		//       a**
		//      /
		//     b*
		//    /
		//   c
		if (nd->left->balance_factor() > 0) {
			rotate_right(nd);
		}
		//       a**
		//      /
		//     b*
		//      \
		//       c
		else {
			rotate_left(nd->left);
			rotate_right(nd);
		}
	}
	// Right side unbalanced
	else {
		//       a**
		//        \
		//        b*
		//         \
		//          c
		if (nd->right->balance_factor() < 0) {
			rotate_left(nd);
		}
		//       a**
		//        \
		//         b*
		//        /
		//       c
		else {
			rotate_right(nd->right);
			rotate_left(nd);
		}
	}
}

constexpr int UNBALANCED = -3;

int check_balanced(Node *node)
{
	if (node == nullptr)
		return -1;

	int left = check_balanced(node->left.get());
	int right = check_balanced(node->right.get());

	if (left == UNBALANCED || right == UNBALANCED)
		return -10;

	int bf = left - right;
	bf = bf > 0 ? bf : -bf;
	if (bf > 1)
		return UNBALANCED;

	return (left > right ? left : right) + 1;
}

int main()
{
	constexpr int N = 64;
	AVLTree tree;
	Timer timeit;

	// ***** Testing *****
	// Sequential data
	//-------------------------------------------
	auto seq_data = gen_seq_ints(0, N);
	cout << "N = " << seq_data.size() << "\n";

	timeit.start();
	for (auto i : seq_data)
		tree.insert(i * 2); // Insert even numbers only.
	timeit.end().print(cout, "Seq  insert");

	// Confirm if balanced
	cout << "Balanced: "
		 << (check_balanced(tree.get_root()) != UNBALANCED ? "YES" : "NO")
		 << "\n";

	timeit.start();
	for (auto i : seq_data)
		assert(tree.search(2 * i));
	timeit.end().print(cout, "Seq  search");

	timeit.start();
	for (auto i : seq_data)
		assert(!tree.search(2 * i + 1));
	timeit.end().print(cout, "Seq !search");

	cout << "\n";
	print_node(cout, tree.get_root());
}
