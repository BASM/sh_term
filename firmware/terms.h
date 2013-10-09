
#define TERM_I2C_PREF 0x48
#define EE_FROM_TERM(termnum) (100+termnum*3)
typedef struct {
	uint8_t num;
	int8_t min;
	int8_t max;
	int16_t cur;
	int8_t last;
	uint8_t port;
} terms;

#define TERM_MAX (0xffff>>1)
typedef int16_t Term;

#define TERMS 8
extern terms term_base_int;
extern terms term_base_ext;
extern terms terms_extend[]; 

void TermUpdate(terms* T, int16_t cur);
