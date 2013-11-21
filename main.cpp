#include <stdio.h>
#include <iostream>
#include "xml.h"

/* Name: Magnus Sundström
 * Data: 2013-11-21
 * TasK: To parse a binary file and create a readable XML-file.
 * Output: The program produces two files - A file with tab-serpareted
 * data and a xml-file.
 */

typedef unsigned char BYTE;

struct field
{
    int totBytes;
    int bytes[115];
    char type[115];
    char name[115][100];
};

field parseFieldStructure()
{
    FILE *pFile = fopen("Field structure.txt", "r");
    char line[100];
    int i, j = 0;
    field fd;
    fd.totBytes = 0;

    while(fgets(line, 100, pFile)) {
        i = -1;
        while(line[++i] != '\t') {
            fd.name[j][i] = line[i];
        }
        fd.name[j][i] = '\0';
        fd.type[j] = line[++i];
        while (line[++i + 1] != '\0') {
            fd.bytes[j] *= 10;
            fd.bytes[j] += line[i] - '0';
        }
        fd.totBytes += fd.bytes[j++];
    }
    fclose (pFile);
    return fd;
}

void bytesToIntger(char* fStr, int fInt)
{
    char tStr[10];
    int j, i = 10;

    if (fInt == 0) {
        fStr[0] = '0';
        fStr[1] = '\0';
        return;
    }

    for (i; i > 0 && fInt != 0; i--) {
        tStr[i - 1] = fInt % 10 + '0';
        fInt /= 10;
    }

    for (j = i; j < 10; j++) {
        fStr[j - i] = tStr[j];
    }

    fStr[j - i] = '\0';
}

void bytesToStr(char* fStr, int &len)
{
    int i ,j = 0;
    fStr[len] = '\0';

    for (i = 0; i < len && fStr[i] == 0x20; i++);

    while(fStr[i] != '\0') {
       fStr[j++] = fStr[i++];
    }

    while(fStr[--i] == 0x20 && i >= 0) {
        fStr[i] = '\0';
    }

    len = 0;
}

int OpenXML(const char* XMLFile, char *attribute)
{
	XML* xml = new XML(XMLFile);
	XMLElement* root = xml->GetRootElement();
	unsigned int nodeCount = root->GetChildrenNum();
	XMLElement** node = root->GetChildren();

	for (unsigned int i = 0; i < nodeCount; i++) {
        unsigned int leafCount = node[i]->GetChildrenNum();
        XMLElement** leaf = node[i]->GetChildren();

        for (unsigned int j = 0; j < leafCount; j++) {
            XMLVariable* Att = leaf[j]->FindVariableZ(attribute);
            if (Att) {
                char Buf[255];
                Att->GetValue(Buf);
                printf("Attribute %s has value %s\n", attribute, Buf);
            }
        }
	}
	delete xml;
	return 0;
}

int main()
{
    FILE *rFile = fopen("Parsefile.txt", "w");
    FILE *bFile = fopen("Samplefile.bin", "rb");
    field fd = parseFieldStructure();

    fseek (bFile, 0, SEEK_END);
    long fSize = ftell (bFile);
    fseek (bFile, 0, SEEK_SET);
	BYTE *buffer = new BYTE[fSize];
	fread(buffer, fSize, 1, bFile);

	int numRec = fSize / fd.totBytes;
    int fieldIdx = 0, fieldIncIdx = 0, intPosIdx = 0, strIdx = 0;
    int fInt = 0;
    char fStr[255];
    bool typeInt = false;

    XML* xml = new XML();
	xml->LoadText("<data></data>");
    XMLElement* root = xml->GetRootElement();
	XMLElement* record[numRec];
	XMLElement* fields[115 * numRec];

    for (int recIdx = 0; recIdx < numRec; recIdx++) {
        record[recIdx] = new XMLElement(root, "record");
        root->InsertElement(recIdx, record[recIdx]);
        bytesToIntger(fStr, recIdx + 1);
        record[recIdx]->AddVariable("id", fStr);

        for (int xmlIdx = 115 * recIdx; xmlIdx < (1 + recIdx) * 115; xmlIdx++) {
            fields[xmlIdx] = new XMLElement(record[recIdx], "field");
            record[recIdx]->InsertElement(xmlIdx, fields[xmlIdx]);
        }
    }

    int xmlIdx = 0;

    for (int idx = 0; idx <= fSize - 1; idx++)
    {
        if(idx >= fieldIncIdx) {

            if(typeInt) {
                bytesToIntger(fStr, fInt);
            } else {
                bytesToStr(fStr, strIdx);
            }

            if(fieldIdx > 0) {
                fields[xmlIdx - 1]->AddVariable(fd.name[fieldIdx - 1], fStr);
            }
            else if(idx != 0) {
                fields[xmlIdx - 1]->AddVariable(fd.name[114], fStr);
            }

            xmlIdx++;
            typeInt = fd.type[fieldIdx] == 'I';

            fprintf(rFile, "%s", fStr);
            fprintf(rFile, "\t");

            fieldIncIdx += fd.bytes[fieldIdx];
            fieldIdx = fieldIdx < 114 ? fieldIdx + 1 : 0;
            if (fieldIdx == 0)
                fprintf(rFile, "\n");
        }

        if(buffer[idx] == 0xe9) {
            fieldIncIdx++;
        } else if(typeInt) {
            fInt <<= 8;
            fInt = (fInt | buffer[idx]);
        } else if (buffer[idx] > 0x1f && buffer[idx] < 0x7f) {
            fStr[strIdx++] = buffer[idx];
        }
    }

    xml->Save("XMLfile.xml");
    fclose(rFile);
    fclose(bFile);

    /* And now we can search for a attrinute
       like subscriber_no in our xml-file. */

    OpenXML("XMLfile.xml", "subscriber_no");

    return 0;
}
