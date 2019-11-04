
int main()
{
    int x = 12;
    int y = 18;
    while (y != 0) {
        if (x > y)
            x -= y;
        else
            y -= x;
    }
    return x;
}
