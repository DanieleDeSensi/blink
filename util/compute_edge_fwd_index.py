#!/usr/bin/env python3
# Computes the edge forwarding index of a graph
import networkx as nx
import itertools

include_external_links = True

def consecutive_pairs(lst):
    pairs = []
    for i in range(len(lst) - 1):
        pairs.append((lst[i], lst[i + 1]))
    return pairs

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

# How many paths cross each edge
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

num_gpus = {}
num_gpus["leonardo"] = 4
num_gpus["lumi"] = 8
num_gpus["alps"] = 4

def get_alps_connectivity():
    ########
    # Alps #
    ########
    bw_per_link = {}
    for i in range(0, num_gpus["alps"]):
        for j in range(0, num_gpus["alps"]):
            if i != j:
                bw_per_link[(i, j)] = 150 
                if include_external_links:
                    # node with id num_gpus["alps"] is the switch
                    bw_per_link[(i, num_gpus["alps"])] = 25 # 1x200Gbit/s NIC per GPU
                    bw_per_link[(num_gpus["alps"], j)] = 25 # 1x200Gbit/s NIC per GPU
    return bw_per_link

def get_leonardo_connectivity():
    ############
    # Leonardo #
    ############
    bw_per_link = {}
    for i in range(0, num_gpus["leonardo"]):
        for j in range(0, num_gpus["leonardo"]):
            if i != j:
                bw_per_link[(i, j)] = 25*4 
                if include_external_links:
                    # node with id num_gpus["leonardo"] is the switch
                    bw_per_link[(i, num_gpus["leonardo"])] = 12.5 # 4x25 GB/s NVLink links + 1x12.5 GB/s Net link
                    bw_per_link[(num_gpus["leonardo"], j)] = 12.5 # 4x25 GB/s NVLink links + 1x12.5 GB/s Net link
    return bw_per_link

def get_lumi_connectivity():
    ########
    # LUMI #
    ########
    bw_per_link = {}
    # GPU 0
    bw_per_link[(0, 1)] = 4*50 # 4 x 50 GB/s
    bw_per_link[(0, 2)] = 1*50 # 1 x 50 GB/s
    bw_per_link[(0, 6)] = 2*50 # 2 x 50 GB/s
    # GPU 1
    bw_per_link[(1, 3)] = 1*50
    bw_per_link[(1, 0)] = 4*50
    bw_per_link[(1, 5)] = 1*50
    # GPU 2
    bw_per_link[(2, 3)] = 4*50
    bw_per_link[(2, 0)] = 1*50
    bw_per_link[(2, 4)] = 2*50
    # GPU 3
    bw_per_link[(3, 1)] = 1*50
    bw_per_link[(3, 7)] = 1*50
    bw_per_link[(3, 2)] = 4*50
    # GPU 4
    bw_per_link[(4, 2)] = 2*50
    bw_per_link[(4, 6)] = 1*50
    bw_per_link[(4, 5)] = 4*50
    # GPU 5
    bw_per_link[(5, 4)] = 4*50
    bw_per_link[(5, 7)] = 1*50
    bw_per_link[(5, 1)] = 1*50
    # GPU 6
    bw_per_link[(6, 4)] = 1*50
    bw_per_link[(6, 0)] = 2*50
    bw_per_link[(6, 7)] = 4*50
    # GPU 7
    bw_per_link[(7, 5)] = 1*50
    bw_per_link[(7, 3)] = 1*50
    bw_per_link[(7, 6)] = 4*50

    if include_external_links:
        # Attach net links    
        # List of net-attached GPUs    
        net_attached = [1, 3, 5, 7]
        for i in range(0, num_gpus["lumi"]):
            for j in range(0, num_gpus["lumi"]):
                if i in net_attached and j in net_attached:
                    # node with id num_gpus["lumi"] is the switch
                    bw_per_link[(i, num_gpus["lumi"])] = 25
                    bw_per_link[(num_gpus["lumi"], j)] = 25

    return bw_per_link

def main():    
    for sys in ['alps', 'leonardo', 'lumi']:
        if sys == 'alps':
            bw_per_link = get_alps_connectivity()
        elif sys == 'leonardo':
            bw_per_link = get_leonardo_connectivity()
        else:
            bw_per_link = get_lumi_connectivity()
        
        # Create a graph
        G = nx.MultiDiGraph()
        edges = []
        for i in range(0, num_gpus[sys]):
            for j in range(0, num_gpus[sys]):
                if i != j:
                    if (i, j) in bw_per_link:
                        #for k in range(0, bw_per_link[(i, j)]): 
                        edges.append((i, j))
        G.add_edges_from(edges)            

        forwarding_index = edge_forwarding_index(G)
        num_rings = count_disjoint_hamiltonian_paths(G)

        min_bw = 99999999
        worst_edge = None
        max_forwarding_index = 0
        for edge, index in forwarding_index.items():
            bw = float(bw_per_link[edge]) / float(forwarding_index[edge])
            if bw < min_bw:
                min_bw = bw
                worst_edge = edge
                max_forwarding_index = forwarding_index[edge]

        # Print edge forwarding index
        #print("Edge Forwarding Index:")
        #for edge, index in forwarding_index.items():
        #    print(f"Edge {edge}: {index}")
        
        # I know that each send from a GPU to another one can go at most at min_bw GB/s
        # I have num_gpus - 1 of those, and I convert them to Gbit/s
        print(f"{sys} peak bw: {min_bw*8*(num_gpus[sys]-1)} Gbit/s. Num rings: {num_rings}. Worst edge: {worst_edge}. Max forwarding index: {max_forwarding_index}")

if __name__ == '__main__':
    main()
