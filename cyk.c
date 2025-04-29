/*
 * cyk.c
 *
 * Tarkistaa Cocke-Younger-Kasami -algoritmilla, onko annettu
 * merkkijono annetun kieliopin mukainen.
 * Kieliopin on oltava yhteydetön ja Chomskyn normaalimuodossa.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void help(char * program_name);

void init();
int load_grammar(char * file_name);
int parse_grammar_line(char * line, int length, char ** error_msg);
int add_production(char start, char end_a, char end_b, char ** error_msg);
void print_grammar();

int string_is_valid(char * str);
int cyk(char * str);
int find_producers(char a, char b, char * results);
void calculate_cell(int i, int k);
void dump_cyk_table();
void print_cyk_table();
inline int table_index(int i, int k);

#define MAX_RULES 25
#define MAX_PRODUCTIONS_PER_SYMBOL 10
#define MAX_RULE_LINE 53

static int n_rules;
static char production_end_symbols[MAX_RULES]
                                  [MAX_PRODUCTIONS_PER_SYMBOL * 2];
static int productions_per_symbol[MAX_RULES];
static char start_symbol;

#define MAX_STRING_LENGTH 80
static char * cyk_table;
static int cyk_table_size; /* in cells */
static int string_length;

int main(int argc, char ** argv)
{
	if (argc == 1) {
		help(argv[0]);
		return 1;
	}
	init();
	if (load_grammar(argv[1]) == 0) {
		return 0;
	}

	char scanf_str[18];
	char str[MAX_STRING_LENGTH + 1];
	sprintf(scanf_str, "%%%ds", MAX_STRING_LENGTH);

	int result;
	while (1) {
		printf("Anna merkkijono tai q lopettaaksesi: ");
		scanf(scanf_str, str);
		if (str[0] == 'q' && str[1] == '\0') {
			break;
		}
		if (string_is_valid(str) != 1) {
			printf("Merkkijonossa saa olla vain kirjaimia a-z.\n\n");
			continue;
		}
		result = 0;
		result = cyk(str);
		printf("Kuuluuko merkkijono kielioppiin: ");
		if (result == 1) {
			printf("kyllä.\n\n");
		}
		else {
			printf("ei.\n\n");
		}
	}
	return 0;
}

void help(char * program_name)
{
	printf(
	"Anna yhteydetön kielioppi Chomskyn normaalimuodossa.\n"
	"Välikemerkit: A-Z  päätemerkit: a-z  tyhjä merkkijono: _\n"
	"Säännöt ovat muotoa \"S -> AB | BB | a\", 1-10 produktiota/rivi.\n"
	"Kirjoita kielioppi tiedostoon ja aja tämä ohjelma uudestaan:\n"
	"%s kielioppitiedosto\n"
	"Ohjelma kysyy kieliopin suhteen testattavat merkkijonot.\n",
	program_name);
}

/* Grammar loading and parsing */

void init()
{
	n_rules = 0;
	int i, j, max_j = MAX_PRODUCTIONS_PER_SYMBOL * 2;
	for (i = 0; i < MAX_RULES; i++) {
		for (j = 0; j < max_j; j++) {
			production_end_symbols[i][j] = ' ';
		}
		productions_per_symbol[i] = 0;
	}
}

int load_grammar(char * file_name)
{
	FILE * fp;
	fp = fopen(file_name, "r");
	if (fp == NULL) {
		printf("Tiedostoa %s ei löydy!\n", file_name);
		return 0;
	}

	size_t max_file_size = MAX_RULES * (MAX_RULE_LINE + 2);
	char * file_buffer = (char *) malloc(max_file_size + 1);
	if (file_buffer == NULL) {
		printf("Liian vähän muistia.\n");
		return 0;
	}
	memset(file_buffer, 0, max_file_size + 1);
	char line_buffer[MAX_RULE_LINE + 1];
	
	fread(file_buffer, max_file_size, 1, fp); 
	if (ferror(fp)) {
		printf("Virhe luettaessa tiedostoa %s.\n", file_name);
		free(file_buffer);
		return 0;
	}
	fclose(fp);

	int file_position = 0;
	int line_position = 0;
	int line_count = 0;
	start_symbol = ' ';
	int parsed_okay = 1;
	char c;
	char * error_msg = NULL;
	while (file_position < max_file_size) {
		c = file_buffer[file_position];
		file_position++;
		if (c == 0x0A || c == 0) {
			line_buffer[line_position] = '\0';
			line_count++;
			if (line_position > 0) {
				if (parse_grammar_line(line_buffer, line_position,
				    &error_msg) == 0)
				{
					parsed_okay = 0;
					break;
				}
			}
			if (c == 0)
				break;
			line_position = 0;
		}
		else {
			line_buffer[line_position] = c;
			line_position++;
			if (line_position == MAX_RULE_LINE) {
				printf("%s: rivi %d on liian pitkä (enintään %d merkkiä)\n",
					file_name, line_count + 1, MAX_RULE_LINE);
				parsed_okay = -1;
				break;
			}
		}
	}
	if (parsed_okay == 1 && line_position > 0) {
		if (parse_grammar_line(line_buffer, line_position, &error_msg) == 0)
			parsed_okay = 0;
	}
	if (parsed_okay == 0) {
		printf("%s: rivi %d on virheellinen", file_name, line_count);
		if (error_msg == NULL) {
			printf(".\n");
		}
		else {
			printf(": %s.\n", error_msg);
		}
	}

	free(file_buffer);
	if (parsed_okay <= 0)
		return 0;
	return 1;
}

static char * pe_not_pair = "pitäisi olla välikepari, oli vain yksi välike";
static char * pe_produces_start_symbol = "mikään välike ei saa tuottaa "
	"aloitusvälikettä";
static char * pe_space_expected = "välilyönti puuttuu jostain kohtaa";
static char * pe_separator_expected = "erotinmerkki '|' puuttuu jostain "
	"kohtaa";
static char * pe_not_single_terminal = "vain yksi päätemerkki per produktio "
	"sallittu, sitten välilyönti, rivinvaihto, tai tiedosto päättyy";
static char * pe_only_start_produces_epsilon = "vain aloitusvälike voi "
	"tuottaa tyhjän merkkijonon";
static char * pe_too_many_per_line = "liikaa produktioita samall välikkeellä";
static char * pe_duplicate_production = "jokin produktio esiintyy useasti";

int parse_grammar_line(char * line, int length, char ** error_msg)
{
	char nonterminal = line[0];
	if (nonterminal < 'A' || nonterminal > 'Z' ||
		strncmp(line + 1, " -> ", 4) != 0)
	{
		return 0;
	}

	if (start_symbol == ' ') {
		start_symbol = nonterminal;
	}

	int position = 5;
	char a, b;
	while (position < length) {
		a = line[position++];
		if (a >= 'A' && a <= 'Z') {
			/* nonterminal-nonterminal pair */
			if (position == length) {
				*error_msg = pe_not_pair;
				return 0;
			}
			b = line[position++];
			if (b < 'A' || b > 'Z') {
				*error_msg = pe_not_pair;
				return 0;
			}
			if (a == start_symbol || b == start_symbol) {
				*error_msg = pe_produces_start_symbol;
				return 0;
			}
			if (position < length) {
				if (line[position++] != ' ') {
					*error_msg = pe_space_expected;
					return 0;
				}
			}
			if (!add_production(nonterminal, a, b, error_msg)) {
				return 0;
			}
		}
		else if ((a >= 'a' && a <= 'z') || a == '_') {
			/* single terminal character or empty string */
			if (a == '_' && nonterminal != start_symbol) {
				*error_msg = pe_only_start_produces_epsilon;
				return 0;
			}
			if (position < length) {
				b = line[position++];
				if (b == ' ') {
				}
				else if (b == 0x0A) {
					b = ' ';
				}
				else {
					*error_msg = pe_not_single_terminal;
					return 0;
				}
			}
			if (!add_production(nonterminal, a, b, error_msg)) {
				return 0;
			}
		}
		else {	
			return 0;
		}

		if (position < length && line[position++] != '|') {
			*error_msg = pe_space_expected;
			return 0;
		}
		if (position < length && line[position++] != ' ') {
			*error_msg = pe_separator_expected;
			return 0;
		}
	}

	return 1;
}

int add_production(char start, char end_a, char end_b, char ** error_msg)
{
	int index = start - 'A';
	int n = productions_per_symbol[index];
	if (n == MAX_PRODUCTIONS_PER_SYMBOL) {
		*error_msg = pe_too_many_per_line;
		return 0;
	}

	int production_type = 1;
	if (end_a >= 'a' && end_a <= 'z')
		production_type = 2;

	int i;
	for (i = 0; i < n; i++) {
		if (production_type == 1) {
			if (production_end_symbols[index][i*2] == end_a &&
			    production_end_symbols[index][i*2+1] == end_b)
			{
				*error_msg = pe_duplicate_production;
				return 0;
			}
		}
		if (production_type == 2) {
			if (production_end_symbols[index][i*2] == end_a) {
				*error_msg = pe_duplicate_production;
				return 0;
			}
		}
	}

	production_end_symbols[index][n*2] = end_a;
	if (production_type == 1)
		production_end_symbols[index][n*2+1] = end_b;
	else
		production_end_symbols[index][n*2+1] = ' ';
	productions_per_symbol[index]++;
	
	return 1;
}

void print_grammar()
{
	printf("Aloitussymboli: %c\n", start_symbol);
	int i, j, n;
	char a, b;
	char * p;
	for (i = 0; i < MAX_RULES; i++) {
		n = productions_per_symbol[i];
		if (n > 0) {
			printf("%c -> ", i + 'A');
			p = production_end_symbols[i];
			for (j = 0; j < n; j++) {
				a = *p++;
				b = *p++;
				if (a <= 'Z')
					printf("%c%c", a, b);
				else
					printf("%c", a);
				if (j != n - 1)
					printf(" | ");
			}
			printf("\n");
		}
	}
}

/* CYK algorithm */

int string_is_valid(char * str)
{
	if (str == NULL || str[0] == '\0') {
		return 0;
	}
	char * p = str;
	while (*p != '\0') {
		if (*p < 'a' || *p > 'z') {
			return 0;
		}
		p++;
	}
	return 1;
}

/* Tests the given string against the loaded grammar.
 * Returns 1 if the string can be derived from the grammar,
 * 0 otherwise.
 */
int cyk(char * str)
{
	if (str == NULL)
		return 0;

	/* Initialize */
	int n = strlen(str);
	if (n > MAX_STRING_LENGTH) {
		printf("Testattava merkkijono saa olla enintään %d merkkiä.\n",
			MAX_STRING_LENGTH);
		return 0;
	}
	string_length = n;
	cyk_table_size = n * (n + 1) / 2;
	cyk_table = (char *) malloc(cyk_table_size * MAX_RULES);
	if (cyk_table == NULL) {
		printf("Ei tarpeeksi muistia. Tarvitaan noin %d kt.\n",
			((cyk_table_size * MAX_RULES) >> 10));
		return 0;
	}
	memset(cyk_table, 0, cyk_table_size * MAX_RULES);

	/* Fill the first row in the CYK table */
	int i, n_producers, j;
	char producers[MAX_RULES + 1];
	int offset = 0;
	for (i = 0; i < string_length; i++) {
		n_producers = find_producers(str[i], ' ', producers);
		if (n_producers == 0) {
			free(cyk_table);
			return 0;
		}
		for (j = 0; j < n_producers; j++) {
			cyk_table[offset + j] = producers[j];
		}
		offset += MAX_RULES;
	}

	/* Fill the other rows */
	int k, max_i;
	for (k = 1; k < string_length; k++) {
		max_i = string_length - k;
		for (i = 0; i < max_i; i++) {
			calculate_cell(i, k);
		}
	}	

	print_cyk_table();

	/* Determine the result and exit */
	int found = 0;
	char * p = cyk_table + (cyk_table_size - 1) * MAX_RULES;
	for (n = 0; n < MAX_RULES && p[n] != 0; n++) {
		if (p[n] == start_symbol) {
			found = 1;
			break;
		}
	}
		
	free(cyk_table);
	return found;
}

/* Finds symbols that produce string '<A-Z><A-Z>' or '<a-z>' or '_'.
 * a and b are the letters of the string.
 * results is a string containing the matching producing symbols.
 * Returns number of matching producing symbols.
 */
int find_producers(char a, char b, char * results)
{
	if ((a >= 'a' && a <= 'z') || a == '_') {
		b = ' ';
	}
	int n_found = 0;
	int i, j, n_j;

	for (i = 0; i < MAX_RULES; i++) {
		n_j = productions_per_symbol[i] * 2;
		j = 0;
		while (j < n_j) {
			if (a == production_end_symbols[i][j] &&
			    b == production_end_symbols[i][j+1])
			{
				results[n_found++] = i + 'A';
				break;
			}
			j += 2;
		}
	}
	results[n_found] = '\0';

	return n_found;
}

/* Calculates a single cell of the CYK table.
 * i = start of the substring. i = 0 .. (n-i)
 * k = length of the substring. k = 1..
 */
void calculate_cell(int i, int k)
{
	int j; /* length of the first half of the substring */
	int first_k, first_i, second_k, second_i;
	char first_cell[MAX_RULES],
	     second_cell[MAX_RULES],
	     target_cell[MAX_RULES];
	int target_cell_length;
	char combinations[MAX_RULES * MAX_RULES * 2];
	int n_combinations, n;
	int s, t;
	char * p;
	char producers[MAX_RULES];
	int n_producers;

	memset(target_cell, 0, MAX_RULES);
	target_cell_length = 0;

	for (j = 0; j < k; j++) {
		/* Determine the first and the second part of
		 * the current substring */
		first_k = j;
		first_i = i;
		second_k = k - 1 - j;
		second_i = i + 1 + j;
		memcpy(first_cell,
			cyk_table + table_index(first_i, first_k) * MAX_RULES,
			MAX_RULES);
		memcpy(second_cell,
			cyk_table + table_index(second_i, second_k) * MAX_RULES,
			MAX_RULES);

		/* Calculate cartesian product between contents of the two cells
	     * in the CYK table. */
		p = combinations;
		s = 0;
		while (first_cell[s] != 0 && s < MAX_RULES) {
			t = 0;
			while (second_cell[t] != 0 && t < MAX_RULES) {
				*p++ = first_cell[s];
				*p++ = second_cell[t];
				t++;
			}
			s++;
		}
		n_combinations = s * t;
	
		/* Seek for every combination against the grammar */	
		p = combinations;
		for (n = 0; n < n_combinations; n++) {
			n_producers = find_producers(p[0], p[1], producers);

			/* Add producers of the combination into the target cell */	
			for (s = 0; s < n_producers; s++) {	
				for (t = 0; t < target_cell_length; t++) {
					if (target_cell[t] == producers[s])
						break;
				}
				if (t == target_cell_length) {
					target_cell[t] = producers[s];
					target_cell_length++;
				}
			}
			p += 2;			
		}
	}

	memcpy(cyk_table + table_index(i, k) * MAX_RULES, target_cell,
		MAX_RULES);
}

void dump_cyk_table()
{
	char * p = cyk_table;
	int i, j;
	printf("dump_cyk_table()\n");
	for (i = 0; i < cyk_table_size; i++) {
		printf("[%04d] ", i);
		for (j = 0; j < MAX_RULES; j++) {
			if (*p != 0) {
				putchar(*p);
			}
			else {
				putchar('~');
			}
			p++;
		}
		putchar('\n');
	}

}

void print_cyk_table()
{
	int column_width[MAX_STRING_LENGTH];
	char line_buffer[MAX_STRING_LENGTH * (MAX_RULES + 1)];
	int i, max_i, k;
	int width;
	char * p;

	/* Calculate width for every column in the table. */
	for (i = 0; i < string_length; i++) {
		column_width[i] = 1;
	}
	/* k is row, i is column */
	for (k = 0; k < string_length; k++) {
		max_i = string_length - k;
		for (i = 0; i < max_i; i++) {
			p = cyk_table + table_index(i, k) * MAX_RULES;
			width = 0;
			while (width < MAX_RULES && p[width] != 0) {
				width++;
			}
			if (width > column_width[i]) {
				column_width[i] = width;
			}
		}
	}

	/* Print table row by row */
	int line_index;
	int t_index = 0;
	for (k = 0; k < string_length; k++) {
		line_index = 0;
		max_i = string_length - k;
		for (i = 0; i < max_i; i++) {
			p = cyk_table + t_index;
			width = 0;
			while (width < MAX_RULES && p[width] != 0) {
				line_buffer[line_index++] = p[width];
				width++;
			}
			if (width == 0) {
				line_buffer[line_index++] = '-';
				width++;
			}
			while (width < column_width[i]) {
				line_buffer[line_index++] = ' ';
				width++;
			}
			line_buffer[line_index++] = ' ';
			t_index += MAX_RULES;
		}
		line_buffer[line_index - 1] = '\0';
		printf("%s\n", line_buffer);
	}
}

/* Converts two-dimensional CYK table index into one-dimensional.
 * Length of the whole string is n.
 * i = start of the substring. i = 0 .. (n-i)
 * k = length of the substring - 1.
 */

inline int table_index(int i, int k)
{
	return k * (string_length - k) + k * (k + 1)/2 + i;
}

