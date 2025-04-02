#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Function to be called before we initiate reboot.
     * By default, this is a weak function hook so that it can be overridden in another file.
     */
    extern void beforeReset();

    /**
     * Reboot because WiFi connection has failed.
     */
    extern void reboot();

#ifdef __cplusplus
}
#endif
