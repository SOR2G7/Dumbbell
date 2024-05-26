# Script para gnuplot
set terminal png
set output "TCPAnal-isis.png"

# Añadir grid y etiquetas para mejor visualización
set grid
set xlabel "Tiempo"
set ylabel "CWND"

# Definición de colores
set style line 1 lt 1 lw 2 lc rgb "red"
set style line 3 lt 1 lw 2 lc rgb "blue"

# Creación de plots
plot "TCPcwnd_nodo2.txt" using 1:2 title "Nodo 2" with lines linestyle 1, \
     "TCPcwnd_nodo3.txt" using 1:2 title "Nodo 3" with lines linestyle 3
