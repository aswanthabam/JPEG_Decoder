class Node:
    def __init__(self, symbol=None, length=None):
        self.symbol = symbol
        self.length = length
        self.left = None
        self.right = None

def build_huffman_tree(symbol_lengths):
    sorted_symbols = sorted(symbol_lengths.items(), key=lambda x: (x[1], x[0]))
    nodes = [Node(symbol=symbol, length=length) for symbol, length in sorted_symbols]

    while len(nodes) > 1:
        left = nodes.pop(0)
        right = nodes.pop(0)
        parent = Node(length=min(left.length, right.length))
        parent.left = left
        parent.right = right
        nodes.append(parent)
        nodes.sort(key=lambda x: (x.length, x.symbol))

    return nodes[0]

# Example usage
symbol_lengths = {'a': 2, 'b': 3, 'c': 3, 'd': 4}
huffman_tree = build_huffman_tree(symbol_lengths)
