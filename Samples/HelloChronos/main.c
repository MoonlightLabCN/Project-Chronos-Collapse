/*
 * The Runtime publishes chronos_add as a deliberately small host ABI for the
 * first MVP.  Later versions can add logging and named UI actions without
 * exposing unrestricted Windows APIs to authored modules.
 */
extern int chronos_add(int first, int second);

int chronos_main(void)
{
    return chronos_add(19, 23);
}
