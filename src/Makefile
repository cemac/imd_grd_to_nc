CC      = gcc
CFLAGS  = -O2 -fPIE -fstack-protector-strong -D_FORTIFY_SOURCE=2 -I.
LDFLAGS = -lnetcdf 
PROGRAM = imd_grd_to_nc

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(PROGRAM): $(PROGRAM).o
	$(CC) -o $@ $< $(LDFLAGS)

all: $(PROGRAM)

clean:
	\rm -f $(PROGRAM).o

clean-all: clean
	\rm -f $(PROGRAM)
