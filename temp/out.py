def find_max_happiness(n, m, happiness, edges):
    from collections import defaultdict

    # 构建邻接表
    graph = defaultdict(list)
    for u, v in edges:
        graph[u].append(v)
        graph[v].append(u)

    max_happiness = -1

    # 遍历每个景点作为重点景点 t
    for t in range(n):
        neighbors = graph[t]
        if len(neighbors) < 2:
            continue

        # 找到所有符合条件的路径
        for i in range(len(neighbors)):
            for j in range(i + 1, len(neighbors)):
                k, l = neighbors[i], neighbors[j]
                if l in graph[k]:
                    for p in range(len(neighbors)):
                        for q in range(p + 1, len(neighbors)):
                            k_prime, l_prime = neighbors[p], neighbors[q]
                            if k_prime != k and k_prime != l and l_prime != k and l_prime != l and l_prime in graph[k_prime]:
                                # 计算幸福度值之和
                                current_happiness = happiness[t] + happiness[k] + happiness[l] + happiness[k_prime] + happiness[l_prime]
                                max_happiness = max(max_happiness, current_happiness)

    return max_happiness

# 读取输入
n, m = map(int, input().split())
happiness = list(map(int, input().split()))
edges = [tuple(map(int, input().split())) for _ in range(m)]

# 计算并输出结果
result = find_max_happiness(n, m, happiness, edges)
print(result)