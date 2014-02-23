static char *to_lower (char *str)
{
  char *s = str;
	while (*s){
	  if (isupper (*s))
		*s = tolower (*s);
		s++;
	}
	return str;
}
