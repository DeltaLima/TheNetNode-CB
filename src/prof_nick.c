#ifdef CONVNICK

/* ggf. Nickname setzen. */
BOOLEAN GetNickname(CONNECTION *cp)
{
  PRFHEARD *prf;

  /* TBL durchsuchen. */
  if ((prf = LookupProfil(&proftab, cp->name)) == FALSE)
    /* Kein Eintrag gefunden. */
    return(FALSE);

  /* Frischer Buffer. */
  memset(cp->nickname, 0, sizeof(cp->nickname));

  /* Nur wenn es ein Nick gibt, */
  if (prf->nick[0] != FALSE)
    /* Nickname setzen. */
    strncpy(cp->nickname, prf->nick, NAMESIZE);

  return(TRUE);
}

/* Aktualisiere Profil-Daten. */
void ProfilService(CONNECTION *cp)
{
  PRFHEARD *nicp;

  /* Eintrag suchen/bereitstellen. */
  if ((nicp = LookupProfil(&proftab, cp->name)) == FALSE)
    /* Neuen Eintrag erstellen. */
    nicp = AddProfil(&proftab, cp->name);

  /* Nur wenn gueltiger Eintrag, */
  if (nicp)
    /* Nickname im User-Profil sichern. */
    UpdateNickProfil(&proftab, nicp, cp->name, cp->nickname);
}

/* Nickname im User-Profil sichern. */
void UpdateNickProfil(PROFTAB *nic, PRFHEARD *nicp, const char *name, const char *nick)
{
  /* Nur wenn es einen Nick gibt, */
  if (nick[0] != FALSE)
    /* Nickname sichern. */
    strncpy(nicp->nick, nick, NAMESIZE);

  nicp->name[L2IDLEN] = 0;

  /* Eintrag Aktualisieren. */
  ulink((LEHEAD *)nicp);
  relink((LEHEAD *)nicp, (LEHEAD *)(nic->heardl.tail));
}

#endif /* CONVNICK */

/* End of src/prof_nick.c */
