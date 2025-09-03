import statistics

def leer_fps(nombre_archivo):
    """Lee un archivo de FPS y devuelve una lista de floats."""
    with open(nombre_archivo, "r") as f:
        return [float(line.strip()) for line in f if line.strip()]

def analizar_fps(lista_fps):
    """Devuelve estadísticas básicas de la lista de FPS."""
    return {
        "promedio": statistics.mean(lista_fps),
        "maximo": max(lista_fps),
        "minimo": min(lista_fps),
        "desviacion": statistics.stdev(lista_fps) if len(lista_fps) > 1 else 0
    }

def comparar_archivos(file1, file2):
    fps1 = leer_fps(file1)
    fps2 = leer_fps(file2)

    stats1 = analizar_fps(fps1)
    stats2 = analizar_fps(fps2)

    print(f"\n📊 Resultados para {file1}:")
    for k, v in stats1.items():
        print(f"   {k.capitalize()}: {v:.2f}")

    print(f"\n📊 Resultados para {file2}:")
    for k, v in stats2.items():
        print(f"   {k.capitalize()}: {v:.2f}")

    # Comparación final
    if stats1["promedio"] > stats2["promedio"]:
        print(f"\n✅ El archivo **{file1}** genera mejores FPS en promedio.")
    elif stats2["promedio"] > stats1["promedio"]:
        print(f"\n✅ El archivo **{file2}** genera mejores FPS en promedio.")
    else:
        print("\n⚖️ Ambos archivos generan el mismo FPS promedio.")

if __name__ == "__main__":
    comparar_archivos("fps_log.txt", "fps_log_paralelo.txt")
