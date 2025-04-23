// See: ISO C23 §6.10.3 "Binary resource inclusion"
static const char a[] = {
    #embed "embed.txt" limit(8)
};

int main(void)
{
    return !(sizeof(a) == 8 && a[0] == '1');
}
