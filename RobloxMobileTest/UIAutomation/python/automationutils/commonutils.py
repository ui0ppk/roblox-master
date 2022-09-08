__author__ = 'yiwen'
import smtplib
from email.mime.text import MIMEText
from email.mime.application import MIMEApplication
from email.mime.multipart import MIMEMultipart
from datetime import date


def send_mail(send_from, send_to, subject, text, attfile=None):
    gmail_user = "robloxmobiletest@gmail.com"
    gmail_pwd = "hackproof12"
    DATE_FORMAT = "%d/%m/%Y"
    EMAIL_SPACE = ", "

    assert isinstance(send_to, list)
    msg = MIMEMultipart()
    msg['Subject'] = subject + " %s" % (date.today().strftime(DATE_FORMAT))
    msg['To'] = EMAIL_SPACE.join(send_to)
    msg['From'] = send_from

    msg.attach(MIMEText(text))
    if attfile:
        part = MIMEApplication(open(attfile,"rb").read())
        part.add_header('Content-Disposition', 'attachment', filename=attfile)
        msg.attach(part)
    try:
        server = smtplib.SMTP("smtp.gmail.com", 587)
        server.ehlo()
        server.starttls()
        server.login(gmail_user, gmail_pwd)
        print 'login successful'
        server.sendmail(send_from, send_to, msg.as_string())
        server.close()
        print 'successfully sent the mail'
    except:
        print "failed to send mail"