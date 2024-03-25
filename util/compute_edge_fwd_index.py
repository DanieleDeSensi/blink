#!/usr/bin/env python3
# Computes the edge forwarding index of a graph
import networkx as nx
import itertools

def consecutive_pairs(lst):
    pairs = []
    for i in range(len(lst) - 1):
        pairs.append((lst[i], lst[i + 1]))
    return pairs


def is_valid(graph, v, path, pos):
    if pos == 0:
        return True
    
    if not graph.has_edge(path[pos-1], v):
        return False
    
    if v in path[:pos]:
        return False
    
    return True

def hamiltonian_paths(graph, v, path, paths):
    path.append(v)
    
    if len(path) == len(graph.nodes()):
        paths.append(path.copy())
    else:
        for neighbor in graph.neighbors(v):
            if is_valid(graph, neighbor, path, len(path)):
                hamiltonian_paths(graph, neighbor, path, paths)
    
    path.pop()

def get_all_hamiltonian_paths(graph):
    paths = []
    for node in graph.nodes():
        hamiltonian_paths(graph, node, [], paths)
    # Complete the path
    for path in paths:
        path.append(path[0])
    
    # It consider bidirectional paths, explicitly add the other direction
    for path in paths.copy():
        paths.append(path[::-1])
    return paths

def count_disjoint_hamiltonian_paths_old(graph):
    all_paths = get_all_hamiltonian_paths(graph)
    disjoint_paths = [all_paths[0]]
    for path_a in all_paths:
        valid = True
        for path_b in disjoint_paths:
            if path_a != path_b:
                # Check if the intersection between the paths is empty
                # If not, path_a is not valid
                if bool(set(consecutive_pairs(path_a)).intersection(set(consecutive_pairs(path_b)))):
                    valid = False
                    break
        if valid and path_a not in disjoint_paths: # Avoid adding itself
            disjoint_paths.append(path_a)
    return disjoint_paths

def all_permutations(input_list):
    return list(itertools.permutations(input_list))

def count_disjoint_hamiltonian_paths(graph):
    all_hamiltonian = []
    # Find all the paths from a node (e.g., 0) to all its neighbors
    for neighbor in graph.neighbors(0):
        paths = nx.all_simple_paths(graph, source=0, target=neighbor)
        for path in paths:
            if len(path) == graph.number_of_nodes():
                path = path + [0]
                all_hamiltonian.append(path)
                # Also append the reverse path
                all_hamiltonian.append(path[::-1])
    print("Found: " + str(len(all_hamiltonian)) + " Hamiltonian paths")
    # At this point we have all the hamiltonian paths, now keep only the edge-disjoint ones
    disjoint_paths = [all_hamiltonian[0]] # Not sure if selecting only the first one always works
    #print(graph.number_of_edges(0, 1))
    for path_a in all_hamiltonian:
        valid = True
        for path_b in disjoint_paths:
            if path_a != path_b:
                # Check if the intersection between the paths is empty
                # If not, path_a is not valid
                if bool(set(consecutive_pairs(path_a)).intersection(set(consecutive_pairs(path_b)))):
                    valid = False
                    break
        if valid and path_a not in disjoint_paths: # Avoid adding itself
            disjoint_paths.append(path_a)
    return disjoint_paths

def edge_forwarding_index(graph):
    edge_forwarding_index = {}
    for src in graph.nodes():
        for dst in graph.nodes():
            if src == dst:
                continue
            
            # Find all simple paths between src and dst
            paths = nx.all_shortest_paths(graph, source=src, target=dst)            
            num_paths = 0
            for path in paths:
                num_paths += 1

            # DO NOT remove, it is a generator so we need to create it again
            paths = nx.all_shortest_paths(graph, source=src, target=dst)            
            # Iterate over all paths
            #print(f"Paths from {src} to {dst}:")
            for path in paths:
                # print(path)
                # For each path we found, get the list of edges on that path
                for edge in consecutive_pairs(path):
                    edge_forwarding_index[edge] = edge_forwarding_index.get(edge, 0) + (1.0 / num_paths)   

    return edge_forwarding_index

def get_leonardo_gpus_graph():
    # Create a graph
    G = nx.MultiDiGraph()

    ############
    # Leonardo #
    ############
    leonardo_gpus_edges = []
    for i in range(0, 4):
        for j in range(0, 4):
            if i != j:
                leonardo_gpus_edges.append((i, j))
    G.add_edges_from(leonardo_gpus_edges)   
    return G

def get_lumi_gpus_graph():
    # Create a graph
    G = nx.MultiDiGraph()

    ########
    # LUMI #
    ########
    lumi_gpus_edges = []
    links_per_pair = {}
    # GPU 0
    links_per_pair[(0, 1)] = 4 # 4 x 50 GB/s
    links_per_pair[(0, 2)] = 1 # 1 x 50 GB/s
    links_per_pair[(0, 6)] = 2 # 2 x 50 GB/s
    # GPU 1
    links_per_pair[(1, 3)] = 1
    links_per_pair[(1, 0)] = 4
    links_per_pair[(1, 5)] = 1
    # GPU 2
    links_per_pair[(2, 3)] = 4
    links_per_pair[(2, 0)] = 1
    links_per_pair[(2, 4)] = 2
    # GPU 3
    links_per_pair[(3, 1)] = 1
    links_per_pair[(3, 7)] = 1
    links_per_pair[(3, 2)] = 4
    # GPU 4
    links_per_pair[(4, 2)] = 2
    links_per_pair[(4, 6)] = 1
    links_per_pair[(4, 5)] = 4
    # GPU 5
    links_per_pair[(5, 4)] = 4
    links_per_pair[(5, 7)] = 1
    links_per_pair[(5, 1)] = 1
    # GPU 6
    links_per_pair[(6, 4)] = 1
    links_per_pair[(6, 0)] = 2
    links_per_pair[(6, 7)] = 4
    # GPU 7
    links_per_pair[(7, 5)] = 1
    links_per_pair[(7, 3)] = 1
    links_per_pair[(7, 6)] = 4
    # List of net-attached GPUs
    net_attached = [1, 3, 5, 7]
    for i in range(0, 8):
        for j in range(0, 8):
            if i != j:
                # Attach net links                
                if i in net_attached and j in net_attached:
                    lumi_gpus_edges.append((i, j))
                if (i, j) in links_per_pair:
                    # Because net links are 25 GB/s links whereas Infinity Fabric links are 50 GB/s links,
                    # we need to double the number of links for the internal links so that everything is 25 GB/s.
                    for k in range(0, links_per_pair[(i, j)]*2):
                        lumi_gpus_edges.append((i, j))
    G.add_edges_from(lumi_gpus_edges)
    return G     

link_bw = {} # Gb/s
link_bw["leonardo"] = (100*8*3 + 100) # 100 GB/s intra (*3) + 100 Gb/s towards the net
link_bw["lumi"] = 25*8*13 # 25 GB/s (*13 -- 12 internal, 1 to the net) (we normalize everything to the net bw so that we can also consider net links)

def main():
    # Add edges to the graph
    #G.add_edges_from([(1, 2), (1, 3), (3, 4)])
    
    for sys in ['leonardo', 'lumi']:
        if sys == 'leonardo':
            G = get_leonardo_gpus_graph()
        else:
            G = get_lumi_gpus_graph()

        forwarding_index = edge_forwarding_index(G)
        min_forwarding_index = min(forwarding_index.values())
        max_forwarding_index = max(forwarding_index.values())
        num_rings = 0 #count_disjoint_hamiltonian_paths(G)

        # Print edge forwarding index
        print("Edge Forwarding Index:")
        for edge, index in forwarding_index.items():
            print(f"Edge {edge}: {index}")
        print(f"{sys} min forwarding index: {min_forwarding_index}, max forwarding index: {max_forwarding_index}. Peak bw: {link_bw[sys] / max_forwarding_index} Gbit/s. Num rings: {num_rings}")

if __name__ == '__main__':
    main()
