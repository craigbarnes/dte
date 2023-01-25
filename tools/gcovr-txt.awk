FNR <= 3 {
    next
}

/^---/ {
    print $0
    next
}

{
    printf("%-35s %10s %10s %10s\n", $1, $2, $3, $4)
}
