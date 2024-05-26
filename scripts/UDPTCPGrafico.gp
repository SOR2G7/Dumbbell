# Script para gnuplot
set terminal png
set output "UDPTCPAnal-isis.png"

# Añadir grid y etiquetas para mejor visualización
set grid
set xlabel "Tiempo"
set ylabel "CWND"

# Definición de colores
set style line 1 lt 1 lw 2 lc rgb "red"
set style line 2 lt 1 lw 2 lc rgb "blue"

# Creación de plots
plot "UDPTCPcwnd_nodo2.txt" using 1:2 title "Nodo 2" with lines linestyle 1, \
     "UDPTCPcwnd_nodo4.txt" using 1:2 title "Nodo 4" with lines linestyle 2
