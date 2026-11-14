import sys,collections
def load(p):
    d=collections.OrderedDict()
    for line in open(p,newline='').read().replace('\r','').split('\n'):
        if ' = ' in line:
            k,v=line.split(' = ',1); d[k]=v
    return d
ref=load('vb6-reference.txt'); w=load('statement-probe.txt')
bad=[(k,ref[k],w.get(k,'<missing>')) for k in ref if ref[k]!=w.get(k,'<missing>')]
print(len(ref),'reference;',len(w),'wfc;',len(bad),'differ')
for k,a,b in bad[:int(sys.argv[1]) if len(sys.argv)>1 else 400]:
    print('##',k); print('  VB6:',a[:400]); print('  WFC:',b[:400])
